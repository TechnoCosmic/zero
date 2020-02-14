//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_SOFTWARETX_H
#define TCRI_ZERO_SOFTWARETX_H


#include <stdint.h>
#include "thread.h"
#include "gpio.h"


namespace zero {

    // Example software serial thread, written as a class.
    // Class-based Threads are easier to make re-entrant,
    // as all 'global' data should be held inside your
    // decendant class.
    class SoftwareTx : public Thread {
    public:
        // ctor
        SoftwareTx(
            const char* const name,
            const PinField txPins );

    private:
        int main();
        const PinField _txPins;
    };

}    // namespace zero


#endif