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

        /// @brief Used to specify system resources
        enum class ResourceId {
            /// Hardware Timer 0
            Timer0,

            /// Hardware Timer 1
            Timer1,

            /// Hardware Timer 2
            Timer2,

            /// Hardware Timer 3
            Timer3,

            /// Hardware USART0 RX
            UsartRx0,

            /// Hardware USART1 RX
            UsartRx1,

            /// Hardware USART2 RX
            UsartRx2,

            /// Hardware USART3 RX
            UsartRx3,

            /// Hardware USART0 TX
            UsartTx0,

            /// Hardware USART1 TX
            UsartTx1,

            /// Hardware USART2 TX
            UsartTx2,

            /// Hardware USART3 TX
            UsartTx3,

            /// Hardware SPI
            Spi,

            /// Hardware ADC
            Adc,

            /// Hardware I2C
            I2c,
        };

        bool obtain( const ResourceId r );
        void release( const ResourceId r );

    };    // namespace resource

}    // namespace zero


#endif