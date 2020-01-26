//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_USART


#ifndef TCRI_ZERO_USART_H
#define TCRI_ZERO_USART_H


#include <stdint.h>
#include <avr/io.h>


#ifdef UCSR0B


#include "thread.h"
#include "doublebuffer.h"


namespace zero {

    class UsartTx {
    public:
        UsartTx(const uint8_t deviceNum);
        ~UsartTx();

        void setCommsParams(const uint32_t baud);
        bool enable(Synapse txReadySyn);
        void disable();

        bool transmit(
            const void* buffer,
            const uint16_t sz,
            const bool allowBlock = false);

        #include "usarttx_private.h"
    };


    class UsartRx {
    public:
        UsartRx(const uint8_t deviceNum);
        ~UsartRx();

        void setCommsParams(const uint32_t baud);
        bool enable(const uint16_t bufferSize, Synapse dataRecdSyn, Synapse overflowSyn);
        void disable();
        uint8_t* getCurrentBuffer(uint16_t& numBytes);
        void flush();
    
        #include "usartrx_private.h"
    };

}


#endif


#endif


#endif
