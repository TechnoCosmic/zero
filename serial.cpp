//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <string.h>
#include "serial.h"


zero::Transmitter& operator<<(zero::Transmitter& out, const char* s)
{
    out.transmit(s, strlen(s));
    return out;
}
