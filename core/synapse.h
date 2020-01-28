//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_SYNAPSE_H
#define TCRI_ZERO_SYNAPSE_H


#include <stdint.h>
#include "thread.h"


namespace zero {

    // the target of a signal. Is a Thread/SignalField pair.
    class Synapse {
    public:
        Thread* thread;
        SignalField signals;

        Synapse();
        Synapse(const SignalField sigs);
        void clear();
        bool isValid() const;
        void signal() const;
        void clearSignals() const;
        void wait() const;
    }; 

}


#endif
