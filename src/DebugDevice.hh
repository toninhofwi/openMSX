// $Id$

#ifndef __DEBUG_DEVICE_
#define __DEBUG_DEVICE_

#include <fstream>
#include <memory>
#include "MSXDevice.hh"

using std::ostream;
using std::ofstream;
using std::auto_ptr;

namespace openmsx {

class EmuTime;
class FilenameSetting;

class DebugDevice : public MSXDevice
{
public:
	DebugDevice(const XMLElement& config, const EmuTime& time);
	virtual ~DebugDevice();
	
	virtual void writeIO(byte port, byte value, const EmuTime& time);
	void openOutput(const string& name);
	void closeOutput(const string& name);
	
	enum DisplayType {HEX, BIN, DEC, ASC};
	enum DebugMode {OFF, SINGLEBYTE, MULTIBYTE, ASCII};

private:
	void outputSingleByte(byte value, const EmuTime& time);
	void outputMultiByte(byte value);
	void displayByte(byte value, DisplayType type);
	
	enum DebugMode mode;
	byte modeParameter;
	auto_ptr<FilenameSetting> fileNameSetting;
	ostream* outputstrm;
	ofstream debugOut;
	string fileNameString;
};

} // namespace openmsx

#endif
