//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "serial.h"


using namespace zero;


// convert the data byte into a formatted UART word (meaning: adds start and stop bits)
uint16_t zero::formatForSerial(const uint8_t d)
{
    uint16_t rc = d << 1;

    rc &= ~(1L << 0);                                   // force start bit low
    rc |= (1L << 9);                                    // stop bit high (so it ends high)

    return rc;
}
