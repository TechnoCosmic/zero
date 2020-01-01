//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#ifndef TCRI_ZERO_USART_H
#define TCRI_ZERO_USART_H


#include <stdint.h>
#include <avr/io.h>


#ifdef UCSR0B


#include "thread.h"
#include "doublebuffer.h"
#include "serial.h"


namespace zero {

    class UsartTx  : public Transmitter {
    public:
        UsartTx(const uint8_t deviceNum);
        ~UsartTx();

        void setCommsParams(const uint32_t baud);
        bool enable(const Synapse txCompleteSyn) override;
        void disable() override;
        bool transmit(const void* buffer, const uint16_t sz) override;

        bool getNextTxByte(uint8_t& data);
        void byteTxComplete();
        
        uint8_t _deviceNum = 0;
        uint8_t* _txBuffer = 0UL;
        uint16_t _txSize = 0UL;
        Synapse _txCompleteSyn;
    };


    class UsartRx : public Receiver {
    public:
        UsartRx(const uint8_t deviceNum);
        ~UsartRx();

        void setCommsParams(const uint32_t baud);
        bool enable(const uint16_t bufferSize, const Synapse dataRecdSyn, const Synapse overflowSyn) override;
        void disable() override;
        uint8_t* getCurrentBuffer(uint16_t& numBytes) override;

        uint8_t _deviceNum = 0;
        DoubleBuffer* _rxBuffer = 0UL;
        Synapse _rxDataReceivedSyn;
        Synapse _rxOverflowSyn;
    };

}


#endif


#endif
