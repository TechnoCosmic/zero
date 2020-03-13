//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_POWER_H
#define TCRI_ZERO_POWER_H


#include <stdint.h>
#include <avr/sleep.h>


namespace zero {

    enum ResetFlags {
        Unknown = 0,
        PowerOn = ( 1 << 0 ),
        External = ( 1 << 1 ),
        Brownout = ( 1 << 2 ),
        Wdt = ( 1 << 3 ),
        Jtag = ( 1 << 4 ),
    };

    class Power {
    public:
        static bool init();
        static ResetFlags getResetFlags();              // determines what caused the last reset

        static void allowSleep();
        static void preventSleep();
        static bool isSleepEnabled();

        static void shutdown(                           // puts the MCU into deep sleep
            const bool force = false,                   // force the shutdown, even with SleepInhibitors present
            const bool silent = false);                 // if true, won't call onSleep()

        static void idle(                               // puts the MCU into idle mode
            const bool force = false,                   // force the idle sleep, even with SleepInhibitors present
            const bool silent = false );                // if true, won't call onSleep()
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
