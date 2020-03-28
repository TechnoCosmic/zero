//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_SUART


#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/atomic.h>

#include "resource.h"
#include "thread.h"
#include "memory.h"
#include "doublebuffer.h"
#include "suart.h"


using namespace zero;


namespace {
    SuartTx* _suartTx{ nullptr };
}


// ctor
SuartTx::SuartTx()
{
    if ( resource::obtain( resource::ResourceId::Timer2 ) ) {
        _suartTx = this;
    }
}


// dtor
SuartTx::~SuartTx()
{
    if ( *this ) {
        disable();
        _suartTx = nullptr;
        resource::release( resource::ResourceId::Timer2 );
    }
}


// validity checking
SuartTx::operator bool() const
{
    return ( _suartTx == this );
}


// starts the periodic bit-timer for transmission
void SuartTx::startTxTimer() const
{
    const uint16_t scaledMs{ (uint16_t) ( F_CPU / ( 16UL * _baud ) ) - 1 };

    TCCR2B = 0;                                         // make sure timer is stopped
    TCNT2 = 0;                                          // reset the counter
    TCCR2A = ( 1 << WGM21 );                            // CTC
    OCR2A = ( scaledMs / 2 ) - 1;                       // scaled for F_CPU
    TIMSK2 |= ( 1 << OCIE2A );                          // enable Timer2 ISR
    TCCR2B = ( ( 1 << CS21 ) | ( 1 << CS20 ) );         // start Timer2 with pre-scaler 32
}


// stops the bit-timer
void SuartTx::stopTxTimer() const
{
    TIMSK2 &= ~( 1 << OCIE2A );                         // disable Timer ISR
    TCCR2B = 0;                                         // make sure timer is stopped
    TCNT2 = 0;                                          // reset the counter
}


// Sets the communications parameters for the software transmitter
void SuartTx::setCommsParams(
    const uint32_t baud,                                // the speed of the communications
    Gpio& pin )                                         // the Gpio object to use for the TX line
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        disable();

        _baud = baud;
        _gpio = &pin;
    }
}


// Enables the software transmitter
bool SuartTx::enable( Synapse& txReadySyn )
{
    if ( !txReadySyn ) return false;

    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        _gpio->setAsOutput();
        _gpio->switchOn();

        power_timer2_enable();                          // power the Timer

        _txReadySyn = &txReadySyn;
        _txReadySyn->signal();

        return true;
    }
}


// disables the software transmitter
void SuartTx::disable()
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        stopTxTimer();
        power_timer2_disable();                         // depower the Timer

        if ( _gpio ) {
            _gpio->reset();
        }

        if ( _txReadySyn ) {
            _txReadySyn->clearSignals();
            _txReadySyn = nullptr;
        }
    }
}


// Transmit a buffer via the software TX pin
bool SuartTx::transmit(
    const void* buffer,
    const uint16_t sz,
    const bool allowBlock )
{
    if ( allowBlock and _txReadySyn ) {
        _txReadySyn->wait();
    }

    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        if ( _txBuffer ) return false;
        if ( !buffer ) return false;
        if ( !sz ) return false;

        if ( _txReadySyn ) {
            _txReadySyn->clearSignals();
        }

        // remember the buffer data
        _txBuffer = (uint8_t*) buffer;
        _txBytesRemaining = sz;

        // enable the ISR that starts the transmission
        startTxTimer();

        return true;
    }
}


// Gets the next byte from the transmission buffer, if there is one
bool SuartTx::getNextTxByte( uint8_t& data )
{
    bool rc{ false };

    data = 0;

    if ( _txBytesRemaining ) {
        data = *_txBuffer++;
        _txBytesRemaining--;

        rc = true;
    }

    return rc;
}


void SuartTx::onTick()
{
    // just in time fetch of data to send
    if ( !_txReg ) {
        // Switch off transmission, even if there are more bytes. We will reset and
        // restart the timer if we need to transmit more data. This improves transmission
        // accuracy when the MCU is experiencing a lot of task switching and ISRs are
        // being switched on and off and so on. This helps prevent bit errors under load,
        // but doesn't completely eliminate them.
        stopTxTimer();

        // the next byte to send is fetched by reference
        uint8_t nextByte;

        if ( !getNextTxByte( nextByte ) ) {
            // no more data to send? tidy up, and signal readiness to go again
            _txBuffer = nullptr;

            if ( _txReadySyn ) {
                _txReadySyn->signal();
            }
        }
        else {
            // load next byte
            _txReg = nextByte << 1;
            _txReg &= ~( 1L << 0 );                     // force start bit low
            _txReg |= ( 1L << 9 );                      // stop bit high (so it ends high)

            startTxTimer();
        }
    }

    if ( _txReg ) {
        // we're mid-byte, keep pumping out the bits
        if ( _txReg & 1 ) {
            _gpio->switchOn();
        }
        else {
            _gpio->switchOff();
        }

        _txReg >>= 1;
    }
}


// Timer tick ISR for the bit-clock
ISR( TIMER2_COMPA_vect )
{
    if ( _suartTx ) {
        _suartTx->onTick();
    }
}


#endif
