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

    /// @brief Provides a driver for accessing the hardware USART transmitters
    /// @code
    /// int hardwareTxDemoThread()
    /// {
    ///     Synapse txReadySyn;
    ///     UsartTx tx{ 0, 9600, txReadySyn };
    ///
    ///     if ( tx ) {
    ///         while ( true ) {
    ///             tx.transmit( "Hello Beaker\r\n", 14, true );
    ///             me.delay( 1_secs );
    ///         }
    ///     }
    /// }
    /// @endcode
    class UsartTx {
    public:
        UsartTx(
            const uint8_t deviceNum,
            const uint32_t baud,
            Synapse& txReadySyn );

        bool transmit(
            const void* buffer,
            const uint16_t sz,
            const bool allowBlock = false );

        explicit operator bool() const;

        #include "usarttx_private.h"
    };

    /// @brief Provides a driver for accessing the hardware USART receivers
    class UsartRx {
    public:
        UsartRx( const uint8_t deviceNum );

        void setCommsParams( const uint32_t baud );

        bool enable(
            const uint16_t bufferSize,
            Synapse& dataRecdSyn,
            Synapse* overflowSyn );

        void disable();
        uint8_t* getCurrentBuffer( uint16_t& numBytes );
        void flush();

        explicit operator bool() const;

        #include "usartrx_private.h"
    };

}    // namespace zero


#endif


#endif


#endif
