/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_USART_H
#define TCRI_ZERO_USART_H

#include <stdint.h>
#include "pipe.h"

namespace zero {

    class Usart {
    public:
        Usart(const uint32_t baud, Pipe* rx, Pipe* tx);
        Pipe* getRxPipe();
        Pipe* getTxPipe();

    private:
        Pipe* _rx;
        Pipe* _tx;
    };

}

zero::Usart& operator<<(zero::Usart& out, const char c);
zero::Usart& operator<<(zero::Usart& out, const char* s);

#endif