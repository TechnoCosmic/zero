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
#include "gpio.h"


using namespace zero;


// Because of the conditional nature of the debug functions,
// we will suppress obvious unused-parameter warnings when
// debug is disabled

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"


namespace {

#ifdef DEBUG_ENABLED

    const int DEBUG_DELAY{ ( 10000 / ( DEBUG_BAUD / 100 ) ) };
    Gpio* _debugPin{ nullptr };

#endif

}    // namespace


void debug::init()
{
#ifdef DEBUG_ENABLED
    _debugPin = new Gpio{ DEBUG_PIN };
    _debugPin->setAsOutput();
    _debugPin->switchOn();
#endif
}


// Transmit a single byte via software bit-banging and no ISRs
void debug::print( const char d )
{
#ifdef DEBUG_ENABLED
    // setup the output 'register'
    uint16_t reg( d << 1 );
    reg &= ~( 1 << 0 );                                 // force start bit low
    reg |= ( 1 << 9 );                                  // stop bit high (so it ends high)

    // stop interrupts because timing is critical
    const uint8_t oldSreg = SREG;
    cli();

    while ( reg ) {
        if ( reg & 1 ) {
            _debugPin->switchOn();
        }
        else {
            _debugPin->switchOff();
        }

        reg >>= 1;

        // 52us = 19200bps, 104us = 9600bps
        _delay_us( DEBUG_DELAY );
    }

    SREG = oldSreg;
#endif
}


// Transmits a NULL-terminated string via software TX
void debug::print( const char* s, const bool fromFlash )
{
#ifdef DEBUG_ENABLED
    while ( true ) {
        char c{ *s };

        if ( fromFlash ) {
            c = pgm_read_byte( s );
        }

        if ( !c ) {
            break;
        }

        debug::print( c );
        s++;
    }
#endif
}


void debug::print( const uint16_t n, const int base )
{
#ifdef DEBUG_ENABLED
    char buffer[ 18 ];
    debug::print( itoa( n, buffer, base ) );
#endif
}


void debug::assert( const bool v, const char* const msg, const int lineNumber )
{
#ifdef DEBUG_ENABLED
    if ( !v ) {
        const char* const tName{ me.getName() };

        if ( tName ) {
            debug::print( tName, true );
            debug::print( PSTR( " - assert fail, line " ), true );
        }
        else {
            debug::print( PSTR( "Assert fail, line " ), true );
        }

        debug::print( lineNumber, 10 );
        debug::print( PSTR( ": " ), true );
        debug::print( msg, true );
        debug::print( PSTR( "\r\n" ), true );
    }
#endif
}


#pragma GCC diagnostic pop
