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

    class Synapse {
    public:
        Synapse();
        ~Synapse();

        explicit operator bool() const;
        operator zero::SignalBitField() const;

        void signal() const;
        void clearSignals() const;
        SignalBitField wait( const uint32_t timeoutMs = 0UL ) const;

        #include "synapse_private.h"
    };

}    // namespace zero


#endif