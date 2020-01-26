//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "debug.h"


using namespace zero;


#ifdef DEBUG_ENABLED


namespace {
    const int DEBUG_PIN_MASK = (1 << DEBUG_PIN);
    const int DEBUG_DELAY = (10000UL / (DEBUG_BAUD / 100));
}


#endif


void debug::init()
{
#ifdef DEBUG_ENABLED
    DEBUG_DDR |= DEBUG_PIN_MASK;
    DEBUG_PORT |= DEBUG_PIN_MASK;
#endif
}


// Transmit a single byte via software bit-banging and no ISRs
void debug::print(const char d)
{
#ifdef DEBUG_ENABLED

    // setup the output 'register'
    uint16_t reg = d << 1;
    reg &= ~(1L << 0);                                   // force start bit low
    reg |= (1L << 9);                                    // stop bit high (so it ends high)

    // stop interrupts because timing is critical
    const uint8_t oldSreg = SREG;
    cli();

    while (reg) {
        if (reg & 1) {
            DEBUG_PORT |= DEBUG_PIN_MASK;
        }
        else {
            DEBUG_PORT &= ~DEBUG_PIN_MASK;
        }

        // next bit please
        reg >>= 1;

        // 52us = 19200bps, 104us = 9600bps
        _delay_us(DEBUG_DELAY);
    }

    // maybe restore interrupts
    SREG = oldSreg;

#endif
}


// Transmits a NULL-terminated string via software TX
void debug::print(const char* s, const bool fromFlash)
{
#ifdef DEBUG_ENABLED

    while (true) {
        char c = *s;

        if (fromFlash) {
            c = pgm_read_byte(s);
        }

        if (!c) {
            break;
        }

        debug::print((char) c);
        s++;
    }

#endif
}


void debug::print(const uint16_t n, const int base)
{
#ifdef DEBUG_ENABLED
    char buffer[18];
    debug::print(itoa(n, buffer, base));
#endif
}
