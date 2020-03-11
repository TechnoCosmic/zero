//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_WDT


#ifndef TCRI_ZERO_WATCHDOG_H
#define TCRI_ZERO_WATCHDOG_H


#include <stdint.h>


namespace zero {

    typedef uint16_t WatchdogFlags;

    class Watchdog {
    public:
        Watchdog();
        ~Watchdog();

        explicit operator bool() const;

        void pat() const;

        #include "watchdog_private.h"
    };

}    // namespace zero


#endif    // TCRI_ZERO_WATCHDOG_H


#endif    // ZERO_DRIVERS_WDT