//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_DEBUG_H
#define TCRI_ZERO_DEBUG_H


#include <avr/pgmspace.h>


namespace zero {

    /// @brief Provides simple debug logging services
    /// @details The debug functions are intended to be very lightweight methods of
    /// outputting data to a terminal for **debugging purposes**, even on devices lacking
    /// a hardware UART.
    /// @details No set up or initialization is required by your code - just be sure that
    /// the ```makefile``` has the ```DEBUG_*``` settings to your liking, and then call
    /// these functions.
    /// @note These are all blocking calls - control returns to the caller only after the
    /// transmission is complete.
    /// @note Interrupts are disabled while each character is transmitted in a tight-loop
    /// (approximately 1ms per character at 9600bps). Interrupts are enabled between each
    /// character.
    /// @note For asynchronous and better performing data transmission, see
    /// the ```UsartTx``` or ```SuartTx``` classes.

    namespace debug {
        /// @private
        void init();

        void print( const char c );
        void print( const char* s, const bool fromFlash = false );
        void print( const uint16_t n, const int base = 10 );
        void assert( const bool v, const char* const msg, const int lineNumber = 0 );
    };

}    // namespace zero

#ifdef DEBUG_ENABLED
    #define dbg( x ) zero::debug::print( x )
    #define dbg_pgm( x ) zero::debug::print( (char*) ( PSTR( x ) ), true )
    #define dbg_int( x ) zero::debug::print( ( x ), 10 )

    #define dbg_assert( v, msg ) zero::debug::assert( ( v ), PSTR( msg ), __LINE__ );
#else
    #define dbg( x ) ;
    #define dbg_pgm( x ) ;
    #define dbg_int( x ) ;

    #define dbg_assert( v, msg ) ;
#endif


#endif
