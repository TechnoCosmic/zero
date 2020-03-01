//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_RESOURCE_H
#define TCRI_ZERO_RESOURCE_H


#include <stdint.h>


namespace zero {

    namespace resource {

        enum class ResourceId {
            Timer0 = 0,
            Timer1,
            Timer2,
            UsartRx0,
            UsartRx1,
            UsartRx2,
            UsartRx3,
            UsartTx0,
            UsartTx1,
            UsartTx2,
            UsartTx3,
            Spi,
            Adc,
            I2c,
        };

        bool obtain( const ResourceId r );
        void release( const ResourceId r );

    };    // namespace resource

}    // namespace zero


#endif