// $Id$

#include "KeyClick.hh"


KeyClick::KeyClick(short volume, const EmuTime &time)
	: dac(volume, time)
{
	status = false;
}

KeyClick::~KeyClick()
{
}

void KeyClick::reset(const EmuTime &time)
{
	setClick(false, time);
}

void KeyClick::setClick(bool newStatus, const EmuTime &time)
{
	if (newStatus != status) {
		status = newStatus;
		dac.writeDAC((status ? 0xff : 0x80), time);
	}
}
