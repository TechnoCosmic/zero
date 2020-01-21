//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_SERIAL_H
#define TCRI_ZERO_SERIAL_H


#include <stdint.h>
#include "thread.h"


namespace zero {

    class Transmitter {
    public:
        virtual bool enable(Synapse txReadySyn) = 0;
        virtual void disable() = 0;
        virtual bool transmit(const void* data, const uint16_t numBytes) = 0;
    };

    class Receiver {
    public:
        virtual bool enable(const uint16_t bufferSize, Synapse rxSyn, Synapse ovfSyn) = 0;
        virtual void disable() = 0;
        virtual uint8_t* getCurrentBuffer(uint16_t& numBytes) = 0;
        virtual void flush() = 0;
    };

    uint16_t formatForSerial(const uint8_t d);
}


#endif