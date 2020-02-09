//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_PINCHANGEDEMO_H
#define TCRI_ZERO_PINCHANGEDEMO_H


#include <stdint.h>
#include "thread.h"
#include "gpio.h"


namespace zero {


    class PinChangeDemo : public Thread {
    public:
        PinChangeDemo(const PinField pins);

    private:
        int main();
        const PinField _pins;
    };


}


#endif