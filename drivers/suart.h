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
#include "gpio.h"


namespace zero {

    class SuartTx {
    public:
        SuartTx();
        ~SuartTx();

        void setCommsParams(
            const uint32_t baud,                        // the speed of the communications
            Gpio& pin );                                // Gpio object to use for the TX line

        bool enable( Synapse& txReadySyn );
        void disable();

        bool transmit(
            const void* buffer,
            const uint16_t sz,
            const bool allowBlock = false );

        explicit operator bool() const;

        #include "suart_private.h"
    };

}    // namespace zero


#endif


#endif
