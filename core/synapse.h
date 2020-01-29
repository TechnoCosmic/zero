//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_Synapse_H
#define TCRI_ZERO_Synapse_H


#include <stdint.h>
#include "thread.h"


namespace zero {

    class Synapse {
    public:
        Synapse();
        ~Synapse();

        explicit operator bool() const;
        operator zero::SignalField() const;

        void signal() const;
        void clearSignals() const;
        SignalField wait() const;

    private:
        Synapse(const Synapse& s) = delete;
        void operator=(const Synapse& s) = delete;

        Thread* const _thread;
        const SignalField _signals;
    };

}


#endif