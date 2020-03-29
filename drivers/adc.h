//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_ADC


#ifndef TCRI_ZERO_ADC_H
#define TCRI_ZERO_ADC_H


#include <stdint.h>
#include "gpio.h"
#include "thread.h"


namespace zero {

    /// @brief Provides asychronous ADC sampling services
    class Adc {
    public:
        Adc( const Synapse& syn );
        explicit operator bool() const;

        void enable();
        void disable();

        void beginConversion( const uint8_t channel );
        uint16_t getLastConversion() const;

        #include "adc_private.h"
    };

}    // namespace zero


#endif    // TCRI_ZERO_ADC_H


#endif    // ZERO_DRIVERS_ADC
