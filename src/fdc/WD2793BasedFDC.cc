// $Id$

#include "WD2793BasedFDC.hh"


WD2793BasedFDC::WD2793BasedFDC(Device *config, const EmuTime &time)
	: MSXDevice(config, time), MSXFDC(config, time),
	  multiplexer(drives), controller(&multiplexer, time)
{
}

WD2793BasedFDC::~WD2793BasedFDC()
{
}

void WD2793BasedFDC::reset(const EmuTime &time)
{
	controller.reset(time);
}
