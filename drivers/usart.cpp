//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_USART


#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>


#ifdef UCSR0B


#include <util/atomic.h>

#include "resource.h"
#include "thread.h"
#include "memory.h"
#include "doublebuffer.h"
#include "usart.h"


using namespace zero;


#ifndef USART_RX_vect
#define USART_RX_vect USART0_RX_vect
#endif

#ifndef USART_TX_vect
#define USART_TX_vect USART0_TX_vect
#endif

#ifndef USART_UDRE_vect
#define USART_UDRE_vect USART0_UDRE_vect
#endif


volatile uint8_t* _UCSRB_base = &UCSR0B;
volatile uint8_t* _UCSRC_base = &UCSR0C;
volatile uint8_t* _UBRRH_base = &UBRR0H;
volatile uint8_t* _UBRRL_base = &UBRR0L;
volatile uint8_t* _UDR_base = &UDR0;

#define UCSRB( p ) *( (volatile uint8_t*) ( _UCSRB_base + ( p * 8 ) ) )
#define UCSRC( p ) *( (volatile uint8_t*) ( _UCSRC_base + ( p * 8 ) ) )
#define UBRRH( p ) *( (volatile uint8_t*) ( _UBRRH_base + ( p * 8 ) ) )
#define UBRRL( p ) *( (volatile uint8_t*) ( _UBRRL_base + ( p * 8 ) ) )
#define UDR( p ) *( (volatile uint8_t*) ( _UDR_base + ( p * 8 ) ) )


#define TX_BITS ( ( 1 << TXEN0 ) | ( 1 << TXCIE0 ) )
#define RX_BITS ( ( 1 << RXEN0 ) | ( 1 << RXCIE0 ) )


#if defined( UCSR3B )
    const int NUM_DEVICES = 4;

#elif defined( UCSR2B )
    const int NUM_DEVICES = 3;

#elif defined( UCSR1B )
    const int NUM_DEVICES = 2;

#elif defined( UCSR0B )
    const int NUM_DEVICES = 1;

#else
    const int NUM_DEVICES = 0;
#endif


namespace {
    UsartTx* _usartTx[ NUM_DEVICES ];
    UsartRx* _usartRx[ NUM_DEVICES ];
}    // namespace


// ctor
UsartTx::UsartTx( const uint8_t deviceNum )
{
    if ( deviceNum < NUM_DEVICES ) {
        // obtain the resource
        auto resId = (resource::ResourceId)( (uint16_t) resource::ResourceId::UsartTx0 + deviceNum );

        if ( resource::obtain( resId ) ) {
            _deviceNum = deviceNum;
            _usartTx[ deviceNum ] = this;
        }
    }
}


// dtor
UsartTx::~UsartTx()
{
    if ( _usartTx[ _deviceNum ] == this ) {
        disable();
        _usartTx[ _deviceNum ] = nullptr;

        // free the resource
        auto resId = (resource::ResourceId)( (uint16_t) resource::ResourceId::UsartTx0 + _deviceNum );
        resource::release( resId );
    }
}


// validity checking
UsartTx::operator bool() const
{
    return ( _usartTx[ _deviceNum ] == this );
}


// Sets the communications parameters
void UsartTx::setCommsParams( const uint32_t baud )
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        const uint16_t scaledMs{ (uint16_t) ( F_CPU / ( 16UL * baud ) ) - 1 };

        // 8-none-1
        UCSRC( _deviceNum ) |= ( 1 << UCSZ01 ) | ( 1 << UCSZ00 );

        // speed
        UBRRH( _deviceNum ) = (uint8_t) scaledMs >> 8;
        UBRRL( _deviceNum ) = (uint8_t) scaledMs;
    }
}


bool UsartTx::enable( Synapse& txReadySyn )
{
    if ( !txReadySyn ) return false;

    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        UCSRB( _deviceNum ) |= TX_BITS;

        _txReadySyn = &txReadySyn;
        _txReadySyn->signal();

        return true;
    }
}


void UsartTx::disable()
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        if ( _txReadySyn ) {
            _txReadySyn->clearSignals();
            _txReadySyn = nullptr;
        }

        UCSRB( _deviceNum ) &= ~TX_BITS;
    }
}


bool UsartTx::transmit(
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

        // prime the buffer data
        _txBuffer = (uint8_t*) buffer;
        _txBytesRemaining = sz;

        // enable the ISR that starts the transmission
        UCSRB( _deviceNum ) |= ( 1 << UDRIE0 );

        return true;
    }
}


bool UsartTx::getNextTxByte( uint8_t& data )
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


void UsartTx::byteTxComplete()
{
    if ( !_txBytesRemaining and _txBuffer ) {
        _txBuffer = nullptr;

        if ( _txReadySyn ) {
            _txReadySyn->signal();
        }
    }
}


UsartRx::UsartRx( const uint8_t deviceNum )
{
    if ( deviceNum < NUM_DEVICES ) {
        // obtain the resource
        auto resId = (resource::ResourceId)( (uint16_t) resource::ResourceId::UsartRx0 + deviceNum );

        if ( resource::obtain( resId ) ) {
            _deviceNum = deviceNum;
            _usartRx[ deviceNum ] = this;
        }
    }
}


UsartRx::~UsartRx()
{
    if ( _usartRx[ _deviceNum ] == this ) {
        disable();
        _usartRx[ _deviceNum ] = nullptr;

        // free the resource
        auto resId = (resource::ResourceId)( (uint16_t) resource::ResourceId::UsartRx0 + _deviceNum );
        resource::release( resId );
    }
}


// validity checking
UsartRx::operator bool() const
{
    return ( _usartRx[ _deviceNum ] == this );
}


void UsartRx::setCommsParams( const uint32_t baud )
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        const uint16_t scaledMs{ (uint16_t) (F_CPU / ( 16UL * baud ) ) - 1 };

        // 8-none-1
        UCSRC( _deviceNum ) |= ( 1 << UCSZ01 ) | ( 1 << UCSZ00 );

        // speed
        UBRRH( _deviceNum ) = (uint8_t) scaledMs >> 8;
        UBRRL( _deviceNum ) = (uint8_t) scaledMs;
    }
}


bool UsartRx::enable( const uint16_t bufferSize, Synapse& rxSyn, Synapse* ovfSyn )
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        bool rc{ false };

        _rxDataReceivedSyn = nullptr;
        _rxOverflowSyn = nullptr;

        delete _rxBuffer;
        _rxBuffer = nullptr;

        if ( ( _rxBuffer = new DoubleBuffer( bufferSize ) ) ) {
            rc = true;

            _rxDataReceivedSyn = &rxSyn;
            _rxOverflowSyn = ovfSyn;

            UCSRB( _deviceNum ) |= RX_BITS;
        }

        return rc;
    }
}


void UsartRx::disable()
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        UCSRB( _deviceNum ) &= ~RX_BITS;

        delete _rxBuffer;
        _rxBuffer = nullptr;

        if ( _rxDataReceivedSyn ) {
            _rxDataReceivedSyn->clearSignals();
            _rxDataReceivedSyn = nullptr;
        }

        if ( _rxOverflowSyn ) {
            _rxOverflowSyn->clearSignals();
            _rxOverflowSyn = nullptr;
        }
    }
}


uint8_t* UsartRx::getCurrentBuffer( uint16_t& numBytes )
{
    return _rxBuffer->getCurrentBuffer( numBytes );
}


void UsartRx::flush()
{
    _rxBuffer->flush();
}


void UsartRx::onRx( const uint8_t deviceNum, const uint8_t data )
{
    if ( _usartRx[ deviceNum ]->_rxBuffer->write( data ) ) {
        if ( _usartRx[ deviceNum ]->_rxDataReceivedSyn ) {
            _usartRx[ deviceNum ]->_rxDataReceivedSyn->signal();
        }
    }
    else {
        if ( _usartRx[ deviceNum ]->_rxOverflowSyn ) {
            _usartRx[ deviceNum ]->_rxOverflowSyn->signal();
        }
    }
}


#ifdef UCSR0B

ISR( USART_TX_vect )
{
    // last byte complete
    _usartTx[ 0 ]->byteTxComplete();
}


ISR( USART_UDRE_vect )
{
    // need more data
    uint8_t nextByte;

    if ( !_usartTx[ 0 ]->getNextTxByte( nextByte ) ) {
        UCSR0B &= ~( 1 << UDRIE0 );
    }
    else {
        UDR0 = nextByte;
    }
}


ISR( USART_RX_vect )
{
    register volatile uint8_t newByte = UDR0;
    UsartRx::onRx( 0, newByte );
}

#endif


#ifdef UCSR1B

ISR( USART1_TX_vect )
{
    // last byte complete
    _usartTx[ 1 ]->byteTxComplete();
}


ISR( USART1_UDRE_vect )
{
    // need more data
    uint8_t nextByte;

    if ( !_usartTx[ 1 ]->getNextTxByte( nextByte ) ) {
        UCSR1B &= ~( 1 << UDRIE1 );
    }
    else {
        UDR1 = nextByte;
    }
}


ISR( USART1_RX_vect )
{
    register volatile uint8_t newByte = UDR1;
    UsartRx::onRx( 1, newByte );
}

#endif


#ifdef UCSR2B

ISR( USART2_TX_vect )
{
    // last byte complete
    _usartTx[ 2 ]->byteTxComplete();
}


ISR( USART2_UDRE_vect )
{
    // need more data
    uint8_t nextByte;

    if ( !_usartTx[ 2 ]->getNextTxByte( nextByte ) ) {
        UCSR2B &= ~( 1 << UDRIE2 );
    }
    else {
        UDR2 = nextByte;
    }
}


ISR( USART2_RX_vect )
{
    register volatile uint8_t newByte = UDR2;
    UsartRx::onRx( 2, newByte );
}

#endif


#ifdef UCSR3B

ISR( USART3_TX_vect )
{
    // last byte complete
    _usartTx[ 3 ]->byteTxComplete();
}


ISR( USART3_UDRE_vect )
{
    // need more data
    uint8_t nextByte;

    if ( !_usartTx[ 3 ]->getNextTxByte( nextByte ) ) {
        UCSR3B &= ~( 1 << UDRIE3 );
    }
    else {
        UDR3 = nextByte;
    }
}


ISR( USART3_RX_vect )
{
    register volatile uint8_t newByte = UDR3;
    UsartRx::onRx( 3, newByte );
}


#endif


#endif


#endif
