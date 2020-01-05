//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#ifndef TCRI_ZERO_SUART_H
#define TCRI_ZERO_SUART_H


#include <stdint.h>

#include "thread.h"
#include "serial.h"


namespace zero {

    class SuartTx : public Transmitter {
    public:
        SuartTx();
        ~SuartTx();

        void setCommsParams(const uint32_t baud, volatile uint8_t* ddr, volatile uint8_t* port, const uint8_t pin);
        bool enable(const Synapse txCompletSyn) override;
        void disable() override;
        bool transmit(const void* buffer, const uint16_t sz) override;

        #include "suart_private.h"
    };

}


#endif