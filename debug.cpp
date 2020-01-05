//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "debug.h"


using namespace zero;


#ifdef DEBUG_ENABLED


namespace {
    const int DEBUG_MASK = (1 << DEBUG_PIN);
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


void debug::print(const char d)
{
#ifdef DEBUG_ENABLED

    // setup the output 'register'
    uint16_t reg = (d << 1);
    reg &= ~(1 << 0);                           // start bit forced low
    reg |= (1 << 9);                            // stop is high (so that TX remains idle high)

    // stop interrupts because timiing is critical
    const uint8_t oldSreg = SREG;
    cli();

    while (reg) {
        if (reg & 1) {
            DEBUG_PORT |= DEBUG_MASK;

        } else {
            DEBUG_PORT &= ~DEBUG_MASK;
        }

        // next bit please
        reg >>= 1;

        // 104us = 9600bps
        _delay_us(DEBUG_DELAY);
    }

    // maybe restore interrupts
    SREG = oldSreg;

#endif

}


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

        debug::print(c);
        s++;
    }

#endif

}
