

#include "WD2793.hh"
#include "DiskDrive.hh"
#include "Scheduler.hh"
#include "Disk.hh"


namespace openmsx {

WD2793::WD2793(DiskDrive* drive_, const EmuTime& time)
	: drive(drive_)
{
	reset(time);
}

WD2793::~WD2793()
{
}

void WD2793::reset(const EmuTime& /*time*/)
{
	//PRT_DEBUG("WD2793::reset()");
	Scheduler::instance().removeSyncPoint(this);
	fsmState = FSM_NONE;

	statusReg = 0;
	trackReg = 0;
	dataReg = 0;
	directionIn = true;
	
	// According to the specs it nows issues a RestoreCommando (0x03)
	// Afterwards the stepping rate can still be changed so that the
	// remaining steps of the restorecommand can go faster. On an MSX this
	// time can be ignored since the bootstrap of the MSX takes MUCH longer
	// then an, even failing, Restorecommand
	commandReg = 0x03;
	sectorReg = 0x01;
	DRQ = false;
	INTRQ = false;
	immediateIRQ = false;
	//statusReg bit 7 (Not Ready status) is already reset
}

bool WD2793::getDTRQ(const EmuTime& time)
{
	if (((commandReg & 0xF0) == 0xF0) &&
	    (statusReg & BUSY)) {
		// WRITE TRACK && status busy
		if (writeTrack) {
			int ticks = DRQTimer.getTicksTill(time);
			if (ticks >= 15) { // TODO found by trial and error
				DRQ = true;
			}
		} else {
			int pulses = drive->indexPulseCount(commandStart, time);
			if (pulses == 1) {
				writeTrack = true;
			}
		}
		if (drive->indexPulseCount(commandStart, time) >= 2) {
			dataAvailable = 0;
			dataCurrent = 0;
			DRQ = false;
			endCmd();
		}
	}
	//PRT_DEBUG("WD2793::getDTRQ() " << DRQ);
	return DRQ;
}

bool WD2793::getIRQ(const EmuTime& /*time*/)
{
	//PRT_DEBUG("WD2793::getIRQ() " << INTRQ);
	return INTRQ;
}

void WD2793::setIRQ()
{
	INTRQ = true;
}

void WD2793::resetIRQ()
{
	INTRQ = immediateIRQ;
}

void WD2793::setCommandReg(byte value, const EmuTime& time)
{
	//PRT_DEBUG("WD2793::setCommandReg() 0x" << hex << (int)value);
	commandReg = value;
	resetIRQ();
	switch (commandReg & 0xF0) {
		case 0x00: // restore
		case 0x10: // seek
		case 0x20: // step
		case 0x30: // step (Update trackRegister)
		case 0x40: // step-in
		case 0x50: // step-in (Update trackRegister)
		case 0x60: // step-out
		case 0x70: // step-out (Update trackRegister)
			startType1Cmd(time);
			break;

		case 0x80: // read sector
		case 0x90: // read sector (multi)
		case 0xA0: // write sector
		case 0xB0: // write sector (multi)
			startType2Cmd(time);
			break;
			
		case 0xC0: // Read Address
		case 0xE0: // read track
		case 0xF0: // write track
			startType3Cmd(time);
			break;
		
		case 0xD0: // Force interrupt
			startType4Cmd();
			break;
	}
}

byte WD2793::getStatusReg(const EmuTime& time)
{
	if (((commandReg & 0x80) == 0) || ((commandReg & 0xF0) == 0xD0)) {
		// Type I or type IV command 
		statusReg &= ~(INDEX | TRACK00 | HEAD_LOADED | WRITE_PROTECTED);
		if (drive->indexPulse(time)) {
			statusReg |=  INDEX;
		}
		if (drive->track00(time)) {
			statusReg |=  TRACK00;
		}
		if (drive->headLoaded(time)) {
			statusReg |=  HEAD_LOADED;
		}
		if (drive->writeProtected()) {
			statusReg |=  WRITE_PROTECTED;
		}
	} else {
		// Not type I command so bit 1 should be DRQ
		if (getDTRQ(time)) {
			statusReg |=  S_DRQ;
		} else {
			statusReg &= ~S_DRQ;
		}
	}

	if (drive->ready()) {
		statusReg &= ~NOT_READY;
	} else {
		statusReg |=  NOT_READY;
	}

	resetIRQ();
	//PRT_DEBUG("WD2793::getStatusReg() 0x" << hex << (int)statusReg);
	return statusReg;
}

void WD2793::setTrackReg(byte value, const EmuTime& /*time*/)
{
	//PRT_DEBUG("WD2793::setTrackReg() 0x" << hex << (int)value);
	trackReg = value;
}

byte WD2793::getTrackReg(const EmuTime& /*time*/)
{
	return trackReg;
}

void WD2793::setSectorReg(byte value, const EmuTime& /*time*/)
{
	//PRT_DEBUG("WD2793::setSectorReg() 0x" << hex << (int)value);
	sectorReg = value;
}

byte WD2793::getSectorReg(const EmuTime& /*time*/)
{
	return sectorReg;
}

void WD2793::setDataReg(byte value, const EmuTime& time)
{
	//PRT_DEBUG("WD2793::setDataReg() 0x" << hex << (int)value);
	// TODO Is this also true in case of sector write?
	//      Not so according to ASM of brMSX
	dataReg = value;
	if ((commandReg & 0xE0) == 0xA0) {
		// WRITE SECTOR
		dataBuffer[dataCurrent] = value;
		dataCurrent++;
		dataAvailable--;
		if (dataAvailable == 0) {
			PRT_DEBUG("WD2793: Now we call the backend to write a sector");
			try {
				dataCurrent = 0;
				drive->write(sectorReg, dataBuffer,
				             onDiskTrack, onDiskSector,
					     onDiskSide, onDiskSize);
				dataAvailable = onDiskSize;
				if (onDiskTrack != trackReg) {
					// TODO we should wait for 6 index holes
					PRT_DEBUG("WD2793: Record not found");
					statusReg |= RECORD_NOT_FOUND;
					endCmd();
					return;
				}
				assert(onDiskSize == 512);
				// If we wait too long we should also write a
				// partialy filled sector ofcourse and set the
				// correct status bits!
				statusReg &= ~(BUSY | S_DRQ);
				if (!(commandReg & M_FLAG)) {
					//TODO verify this !
					setIRQ();
					DRQ = false;
				}
			} catch (MSXException& e) {
				// Backend couldn't write data
				// TODO which status bit should be set?
				statusReg |= RECORD_NOT_FOUND;
				endCmd();
				return;
			}
		}
	} else if ((commandReg & 0xF0) == 0xF0) {
		// WRITE TRACK
		//PRT_DEBUG("WD2793 WRITE TRACK value "<<hex<<
		//          (int)value<<dec);
		//DRQ related timing
		DRQ = false;
		DRQTimer.advance(time);
		
		//indexmark related timing
		int pulses = drive->indexPulseCount(commandStart, time);
		switch (pulses) {
		case 0: // no index pulse yet
			break;
		case 1: // first index pulse passed
			drive->writeTrackData(value);
			break;
		default: // next indexpulse passed
			dataAvailable = 0; //return correct DTR
			dataCurrent = 0;
			DRQ = false;
			endCmd();
			break;
		}
		/* followin switch stement belongs in the backend, since
		 * we do not know how the actual diskimage stores the
		 * data. It might simply drop all the extra CRC/header
		 * stuff and just use some of the switches to actually
		 * simply write a 512 bytes sector.
		 *
		 * However, timing should be done here :-\
		 *

		 switch (value) {
		 case 0xFE:
		 case 0xFD:
		 case 0xFC:
		 case 0xFB:
		 case 0xFA:
		 case 0xF9:
		 case 0xF8:
			PRT_DEBUG("CRC generator initializing");
			break;
		 case 0xF6:
			PRT_DEBUG("write C2 ?");
			break;
		 case 0xF5:
			PRT_DEBUG("CRC generator initializing in MFM, write A1?");
			break;
		 case 0xF7:
			PRT_DEBUG("two CRC characters");
			break;
		 default:
			//Normal write to track
			break;
		 }
		 //shouldn't been done here !!
		 statusReg &= ~0x03;	// reset status on Busy and DRQ
		 */


		/*
		   if (indexmark) {
		   statusReg &= ~0x03;	// reset status on Busy and DRQ
		   setIRQ();
		   DRQ = false; 
		   }
		 */
	}
}

byte WD2793::getDataReg(const EmuTime& /*time*/)
{
	if (((commandReg & 0xE0) == 0x80) && (statusReg & BUSY)) {
		// READ SECTOR
		dataReg = dataBuffer[dataCurrent];
		dataCurrent++;
		dataAvailable--;
		if (dataAvailable == 0) {
			if (!(commandReg & M_FLAG)) {
				statusReg &= ~(BUSY | S_DRQ);
				DRQ = false;
				setIRQ();
				PRT_DEBUG("WD2793: Now we terminate the read sector command");
			} else {
				// TODO ceck in tech data (or on real machine)
				// if implementation multi sector read is
				// correct, since this is programmed by hart.
				sectorReg++;
				tryToReadSector();
			}
		}
	}
	return dataReg;
}

void WD2793::tryToReadSector()
{
	try {
		drive->read(sectorReg, dataBuffer,
		            onDiskTrack, onDiskSector, onDiskSide, onDiskSize);
		if (onDiskTrack != trackReg) {
			// TODO we should wait for 6 index holes
			statusReg |= RECORD_NOT_FOUND;
			endCmd();
			return;
		}
		assert(onDiskSize == 512);
		dataCurrent = 0;
		dataAvailable = onDiskSize;
		DRQ = true;	// data ready to be read
	} catch (MSXException& e) {
		PRT_DEBUG("WD2793: read sector failed: " << e.getMessage());
		DRQ = false;	// TODO data not ready (read error)
		statusReg = 0;	// reset flags
	}
}


void WD2793::schedule(FSMState state, const EmuTime& time)
{
	Scheduler::instance().removeSyncPoint(this);
	fsmState = state;
	Scheduler::instance().setSyncPoint(time, this);
}

void WD2793::executeUntil(const EmuTime& time, int /*userData*/)
{
	FSMState state = fsmState;
	fsmState = FSM_NONE;
	switch (state) {
		case FSM_SEEK:
			if ((commandReg & 0x80) == 0x00) {
				// Type I command
				seekNext(time);
			}
			break;
		case FSM_TYPE2_WAIT_LOAD:
			if ((commandReg & 0xC0) == 0x80)  {
				// Type II command
				type2WaitLoad(time);
			}
			break;
		case FSM_TYPE2_LOADED:
			if ((commandReg & 0xC0) == 0x80)  {
				// Type II command
				type2Loaded();
			}
			break;
		case FSM_TYPE3_WAIT_LOAD:
			if (((commandReg & 0xC0) == 0xC0) &&
			    ((commandReg & 0xF0) != 0xD0)) { 
				// Type III command
				type3WaitLoad(time);
			}
			break;
		case FSM_TYPE3_LOADED:
			if (((commandReg & 0xC0) == 0xC0) &&
			    ((commandReg & 0xF0) != 0xD0)) { 
				// Type III command
				type3Loaded(time);
			}
			break;
		default:
			assert(false);
	}
}

const string& WD2793::schedName() const
{
	static const string name("WD2793");
	return name;
}

void WD2793::startType1Cmd(const EmuTime& time)
{
	statusReg &= ~(SEEK_ERROR | CRC_ERROR);
	statusReg |= BUSY;
	DRQ = false;

	drive->setHeadLoaded(commandReg & H_FLAG, time);

	switch (commandReg & 0xF0) {
		case 0x00: // restore
			trackReg = 0xFF;
			dataReg  = 0x00;
			seek(time);
			break;
			
		case 0x10: // seek
			seek(time);
			break;
			
		case 0x20: // step
		case 0x30: // step (Update trackRegister)
			step(time);
			break;
			
		case 0x40: // step-in
		case 0x50: // step-in (Update trackRegister)
			directionIn = true;
			step(time);
			break;
			
		case 0x60: // step-out
		case 0x70: // step-out (Update trackRegister)
			directionIn = false;
			step(time);
			break;
	}
}

void WD2793::seek(const EmuTime& time)
{
	if (trackReg == dataReg) {
		endType1Cmd();
	} else {
		directionIn = (dataReg > trackReg);
		step(time);
	}
}

void WD2793::step(const EmuTime& time)
{
	const int timePerStep[4] = {
		// in ms, in case a 1MHz clock is used (as in MSX)
		6, 12, 20, 30
	};

	if ((commandReg & T_FLAG) || ((commandReg & 0xE0) == 0x00)) {
		// Restore or seek  or  T_FLAG
		if (directionIn) {
			trackReg++;
		} else {
			trackReg--;
		}
	}
	if (!directionIn && drive->track00(time)) {
		trackReg = 0;
		endType1Cmd();
	} else {
		drive->step(directionIn, time);
		Clock<1000> next(time);	// ms
		next += timePerStep[commandReg & STEP_SPEED];
		schedule(FSM_SEEK, next.getTime());
	}
}

void WD2793::seekNext(const EmuTime& time)
{
	if ((commandReg & 0xE0) == 0x00) {
		// Restore or seek
		seek(time);
	} else {
		endType1Cmd();
	}
}

void WD2793::endType1Cmd()
{
	if (commandReg & V_FLAG) {
		// verify sequence
		// TODO verify sequence
	}
	endCmd();
}


void WD2793::startType2Cmd(const EmuTime& time)
{
	statusReg &= ~(LOST_DATA   | RECORD_NOT_FOUND |
	               RECORD_TYPE | WRITE_PROTECTED);
	statusReg |= BUSY;
	DRQ = false;

	if (!drive->ready()) {
		endCmd();
	} else {
		// WD2795/WD2797 would now set SSO output
		drive->setHeadLoaded(true, time);

		if (commandReg & E_FLAG) {
			Clock<1000> next(time);	// ms
			next += 30;	// when 1MHz clock
			schedule(FSM_TYPE2_WAIT_LOAD, next.getTime());
		} else {
			type2WaitLoad(time);
		}
	}
}

void WD2793::type2WaitLoad(const EmuTime& time)
{
	// TODO wait till head loaded, I arbitrarily took 1ms delay
	Clock<1000> next(time);
	next += 1;
	schedule(FSM_TYPE2_LOADED, next.getTime());
}

void WD2793::type2Loaded()
{
	if (((commandReg & 0xE0) == 0xA0) && (drive->writeProtected())) {
		// write command and write protected
		PRT_DEBUG("WD2793: write protected");
		statusReg |= WRITE_PROTECTED;
		endCmd();
	} else {
		switch (commandReg & 0xF0) {
			case 0x80: //read sector
			case 0x90: //read sector (multi)
				tryToReadSector();
				break;

			case 0xA0: // write sector
			case 0xB0: // write sector (multi)
				dataCurrent = 0;
				dataAvailable = 512;	// TODO should come from sector header
				DRQ = true;	// data ready to be written
				break;
		}
	}
}

void WD2793::startType3Cmd(const EmuTime& time)
{
	statusReg &= ~(LOST_DATA | RECORD_NOT_FOUND | RECORD_TYPE);
	statusReg |= BUSY;
	DRQ = false;
	writeTrack = false;

	if (!drive->ready()) {
		endCmd();
	} else {
		drive->setHeadLoaded(true, time);
		// WD2795/WD2797 would now set SSO output

		if (commandReg & E_FLAG) {
			Clock<1000> next(time);	// ms
			next += 30;	// when 1MHz clock
			schedule(FSM_TYPE3_WAIT_LOAD, next.getTime());
		} else {
			type3WaitLoad(time);
		}
	}
}

void WD2793::type3WaitLoad(const EmuTime& time)
{
	// TODO wait till head loaded, I arbitrarily took 1ms delay
	Clock<1000> next(time);
	next += 1;
	schedule(FSM_TYPE3_LOADED, next.getTime());
}

void WD2793::type3Loaded(const EmuTime& time)
{

	// TODO TG43 update

	commandStart = time;
	switch (commandReg & 0xF0) {
	case 0xC0: // read Address
		readAddressCmd();
		break;
	case 0xE0: // read track
		readTrackCmd();
		break;
	case 0xF0: // write track
		writeTrackCmd(time);
		break;
	}
}

void WD2793::readAddressCmd()
{
	PRT_DEBUG("WD2793 command: read address  NOT YET IMPLEMENTED");
	endCmd();
}

void WD2793::readTrackCmd()
{
	PRT_DEBUG("WD2793 command: read track   NOT YET IMPLEMENTED");
	endCmd();
}

void WD2793::writeTrackCmd(const EmuTime& time)
{
	PRT_DEBUG("WD2793 command: write track");

	if (drive->writeProtected()) {
		// write track command and write protected
		PRT_DEBUG("WD2793: write protected");
		statusReg |= WRITE_PROTECTED;
		endCmd();
	} else {
		DRQ = true;
		
		// TODO wait for indexPulse

		PRT_DEBUG("WD2793: initWriteTrack()");
		drive->initWriteTrack(); 
		DRQTimer.advance(time);
	}
}

void WD2793::startType4Cmd()
{
	// Force interrupt
	PRT_DEBUG("WD2793 command: Force interrupt");
	
	byte flags = commandReg & 0x0F;
	if ((flags & 0x07) != 0x00) {
		// all flags not yet supported
		PRT_DEBUG("WD2793 type 4 cmd, unimplemented bits " << (int)flags);
	}

	if (flags == 0x00) {
		immediateIRQ = false;
	}
	if (flags & IMM_IRQ) {
		immediateIRQ = true;
	}
	
	DRQ = false;
	statusReg &= ~BUSY;	// reset status on Busy
}

void WD2793::endCmd()
{
	setIRQ();
	statusReg &= ~BUSY;
}

} // namespace openmsx

