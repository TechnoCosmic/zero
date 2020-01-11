//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_DOUBLEBUFFER_H
#define TCRI_ZERO_DOUBLEBUFFER_H


#include <stdint.h>


namespace zero {


    class DoubleBuffer {
    public:
        DoubleBuffer(const uint16_t size);
        ~DoubleBuffer();

        bool write(const uint8_t d);
        uint8_t* getCurrentBuffer(uint16_t& numBytes);
        void flush();

        #include "doublebuffer_private.h"
    };


}


#endif