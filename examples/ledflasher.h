//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_LEDFLASHER_H
#define TCRI_ZERO_LEDFLASHER_H


#include <stdint.h>
#include "thread.h"
#include "gpio.h"


namespace zero {

    // Example LED flasher thread, written as a class.
    // Class-based Threads are easier to make re-entrant,
    // as all 'global' data should be held inside your
    // decendant class.
    class LedFlasher : public Thread {
    public:
        // ctors
        LedFlasher(
            const char* const name,
            const PinField ledPins,
            const uint32_t delayMs,
            int count = 0 );

        LedFlasher(
            const char* const name,
            const PinField ledPins,
            const uint32_t timeOnMs,
            const uint32_t timeOffMs,
            int count );

    private:
        int main();

        const PinField _ledPins;
        const uint32_t _timeOnMs;
        const uint32_t _timeOffMs;
        int _flashesRemaining;
    };

}    // namespace zero


#endif