//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#ifndef TCRI_ZERO_SERIAL_H
#define TCRI_ZERO_SERIAL_H


#include <stdint.h>
#include "thread.h"


namespace zero {

    class Transmitter {
    public:
        virtual bool enable(const Synapse txCompleteSyn) = 0;
        virtual void disable() = 0;
        virtual bool transmit(const void* data, const uint16_t numBytes) = 0;
    };

    class Receiver {
    public:
        virtual bool enable(const uint16_t bufferSize, const Synapse rxSyn, const Synapse ovfSyn) = 0;
        virtual void disable() = 0;
        virtual uint8_t* getCurrentBuffer(uint16_t& numBytes) = 0;
    };

}


zero::Transmitter& operator<<(zero::Transmitter& out, const char* s);


#endif