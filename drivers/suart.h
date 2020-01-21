//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_SUART


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

        void setCommsParams(
            const uint32_t baud,                        // the speed of the communications
            volatile uint8_t* ddr,                      // address of the DDR for the software TX pin
            volatile uint8_t* port,                     // address of the PORT for the software TX pin
            const uint8_t pin);                         // the pin number for the TX (0-7)

        bool enable(const Synapse txReadySyn) override;
        void disable() override;
        bool transmit(const void* buffer, const uint16_t sz) override;

    #include "suart_private.h"
    };

}


#endif


#endif
