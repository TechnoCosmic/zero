//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


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
    DEBUG_DDR |= (1 << DEBUG_PIN);
    DEBUG_PORT |= (1 << DEBUG_PIN);
#endif
}


// Transmit a single byte via software bit-banging and no ISRs
void debug::print(char d)
{
#ifdef DEBUG_ENABLED

    // setup the output 'register'
    uint16_t reg = (d << 1);
    reg &= ~(1 << 0);                           // start bit forced low
    reg |= (1 << 9);                            // stop is high (so that TX remains idle high)

    // stop interrupts because timing is critical
    const uint8_t oldSreg = SREG;
    cli();

    while (reg) {
        if (reg & 1) {
            DEBUG_PORT |= DEBUG_PIN_MASK;

        } else {
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
void debug::print(char* s, const bool fromFlash)
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

        debug::print(c);
        s++;
    }

#endif
}
