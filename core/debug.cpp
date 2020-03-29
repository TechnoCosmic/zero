//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


/// @file
/// @brief Contains functions for debugging purposes


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


/// @brief Transmits a single byte via software TX
/// @param c The character to transmit.
void debug::print( const char c )
{
#ifdef DEBUG_ENABLED
    // setup the output 'register'
    uint16_t reg( c << 1 );
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


/// @brief Transmits a NULL-terminated character array via software TX
/// @param s The null-terminated character array to transmit.
/// @param fromFlash If ```true```, ```s``` points to an address in Flash memory instead of SRAM
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


/// @brief Transmits the ASCII representation of a 16-bit integer via software TX
/// @param n The unsigned number to transmit.
/// @param base Optional. Default: ```10```. The base to convert the number to.
void debug::print( const uint16_t n, const int base )
{
#ifdef DEBUG_ENABLED
    char buffer[ 18 ];
    debug::print( itoa( n, buffer, base ) );
#endif
}


/// @brief Tests a boolean condition and logs a message if ```false```.
/// @param v The result of an expression to test.
/// @param msg A null-terminated character array to transmit via software TX
/// if ```v``` is ```false```.
/// @param lineNumber The source code line number on which the expression was evaluated.
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
