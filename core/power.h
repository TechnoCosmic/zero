//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_POWER_H
#define TCRI_ZERO_POWER_H


#include <stdint.h>


namespace zero {


    /// @brief Used to determine what caused a reset event
    enum ResetFlags {
        /// Unknown
        Unknown = 0,

        /// Normal power-on
        PowerOn = ( 1 << 0 ),

        /// External power event
        External = ( 1 << 1 ),

        /// Brownout reset
        Brownout = ( 1 << 2 ),

        /// Watchdog timeout
        Wdt = ( 1 << 3 ),

        /// JTAG debugger
        Jtag = ( 1 << 4 ),
    };


    /// @brief Provides power, reset, and sleep management services
    class Power {
    public:
        /// @private
        static bool init();

        static ResetFlags getResetFlags();              // Determines what caused the last reset event
        static void allowSleep();                       // Allows the MCU to sleep when asked
        static void preventSleep();                     // Prevents the MCU from entering sleep mode
        static bool isSleepEnabled();                   // Determines if sleeping is currently allowed

        // Sleeps the MCU
        static void sleep(
            const uint8_t mode,
            const bool force = false,
            const bool silent = false);
    };

}


// Funky little ATOMIC_BLOCK macro clones for sleeping
static inline uint8_t __iPreventRetVal()
{
    zero::Power::preventSleep();
    return 1;
}


static inline void __iZeroSleepRestore( const uint8_t* const __tmr_save )
{
    if ( *__tmr_save ) {
        zero::Power::allowSleep();
    }
}


#define ZERO_SLEEP_INHIBIT          for ( uint8_t tmr_save CLEANUP( __iZeroSleepRestore ) = ( uint8_t )( zero::Power::isSleepEnabled() ), __ToDo = __iPreventRetVal(); __ToDo; __ToDo = 0 )


#endif
