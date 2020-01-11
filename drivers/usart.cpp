//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>


#ifdef UCSR0B


#include <util/atomic.h>

#include "../thread.h"
#include "../memory.h"
#include "../doublebuffer.h"
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

#define UCSRB(p) *((volatile uint8_t*) (_UCSRB_base+(p*8)))
#define UCSRC(p) *((volatile uint8_t*) (_UCSRC_base+(p*8)))
#define UBRRH(p) *((volatile uint8_t*) (_UBRRH_base+(p*8)))
#define UBRRL(p) *((volatile uint8_t*) (_UBRRL_base+(p*8)))
#define UDR(p) *((volatile uint8_t*) (_UDR_base+(p*8)))


#if defined(UCSR3B)
    const int NUM_DEVICES = 4;

#elif defined(UCSR2B)
    const int NUM_DEVICES = 3;

#elif defined(UCSR1B)
    const int NUM_DEVICES = 2;

#elif defined(UCSR0B)
    const int NUM_DEVICES = 1;

#else
    const int NUM_DEVICES = 0;
#endif


namespace {
    UsartTx* _usartTx[NUM_DEVICES];
    UsartRx* _usartRx[NUM_DEVICES];
} 


UsartTx::UsartTx(const uint8_t deviceNum)
{
    if (deviceNum < NUM_DEVICES) {
        _deviceNum = deviceNum;
        _usartTx[deviceNum] = this;
    }
}


UsartTx::~UsartTx()
{
    disable();
    _usartTx[_deviceNum] = 0UL;
}


void UsartTx::setCommsParams(const uint32_t baud)
{
    const uint16_t pre = (F_CPU / (16UL * baud)) - 1;

    // 8-none-1
    UCSRC(_deviceNum) |= (1 << UCSZ01) | (1 << UCSZ00);

    // speed
    UBRRH(_deviceNum) = (uint8_t) pre >> 8;
    UBRRL(_deviceNum) = (uint8_t) pre;
}


#define TX_BITS ((1 << TXEN0) | (1 << TXCIE0))


bool UsartTx::enable(Synapse txReadySyn)
{
    if (!txReadySyn.isValid()) return false;

    UCSRB(_deviceNum) |= TX_BITS;

    _txReadySyn = txReadySyn;
    _txReadySyn.signal();

    return true;
}


void UsartTx::disable()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _txReadySyn.clearSignals();
        _txReadySyn.clear();

        UCSRB(_deviceNum) &= ~TX_BITS;
    }
}


bool UsartTx::transmit(const void* buffer, const uint16_t sz)
{
    if (_txBuffer) return false;
    if (!buffer) return false;
    if (!sz) return false;

    _txReadySyn.clearSignals();

    // prime the buffer data
    _txBuffer = (uint8_t*) buffer;
    _txSize = sz;

    // enable the ISR that starts the transmission
    UCSRB(_deviceNum) |= (1 << UDRIE0);

    return true;
}


bool UsartTx::getNextTxByte(uint8_t& data)
{
    bool rc = false;

    data = 0;

    if (_txSize) {
        data = *_txBuffer++;
        _txSize--;

        rc = true;
    }

    return rc;
}


void UsartTx::byteTxComplete() {
    if (!_txSize && _txBuffer != 0UL) {
        _txBuffer = 0UL;
        _txReadySyn.signal();
    }
}


UsartRx::UsartRx(const uint8_t deviceNum)
{
    if (deviceNum < 2) {
        _deviceNum = deviceNum;
        _usartRx[deviceNum] = this;
    }
}


UsartRx::~UsartRx()
{
    disable();
    _usartRx[_deviceNum] = 0UL;
}


void UsartRx::setCommsParams(const uint32_t baud)
{
    const uint16_t pre = (F_CPU / (16UL * baud)) - 1;

    // 8-none-1
    UCSRC(_deviceNum) |= (1 << UCSZ01) | (1 << UCSZ00);

    // speed
    UBRRH(_deviceNum) = (uint8_t) pre >> 8;
    UBRRL(_deviceNum) = (uint8_t) pre;
}


#define RX_BITS ((1 << RXEN0) | (1 << RXCIE0))


bool UsartRx::enable(
    const uint16_t bufferSize,
    Synapse rxSyn,
    Synapse ovfSyn)
{
    bool rc = false;

    _rxDataReceivedSyn.clear();
    _rxOverflowSyn.clear();

    delete _rxBuffer;
    _rxBuffer = 0UL;

    if (_rxBuffer = new DoubleBuffer(bufferSize)) {
        rc = true;

        _rxDataReceivedSyn = rxSyn;
        _rxOverflowSyn = ovfSyn;

        UCSRB(_deviceNum) |= RX_BITS;
    }
    
    return rc;
}


void UsartRx::disable()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        UCSRB(_deviceNum) &= ~RX_BITS;

        delete _rxBuffer;
        _rxBuffer = 0UL;

        _rxDataReceivedSyn.clear();
        _rxOverflowSyn.clear();
    }
}


uint8_t* UsartRx::getCurrentBuffer(uint16_t& numBytes)
{
    return _rxBuffer->getCurrentBuffer(numBytes);
}


ISR(USART_TX_vect)
{
    // last byte complete
    _usartTx[0]->byteTxComplete();
}


ISR(USART_UDRE_vect)
{
    // need more data
    uint8_t nextByte;

    if (!_usartTx[0]->getNextTxByte(nextByte)) {
        UCSR0B &= ~(1 << UDRIE0);

    } else {
        UDR0 = nextByte;
    }
}


ISR(USART_RX_vect)
{
    register volatile uint8_t newByte = UDR0;

    // received data
    if (_usartRx[0]->_rxBuffer->write(newByte)) {
        _usartRx[0]->_rxDataReceivedSyn.signal();

    } else {
        _usartRx[0]->_rxOverflowSyn.signal();
    }
}


#ifdef UCSR1B


ISR(USART1_TX_vect)
{
    // last byte complete
    _usartTx[1]->byteTxComplete();
}


ISR(USART1_UDRE_vect)
{
    // need more data
    uint8_t nextByte;

    if (!_usartTx[1]->getNextTxByte(nextByte)) {
        UCSR1B &= ~(1 << UDRIE1);

    } else {
        UDR1 = nextByte;
    }
}


ISR(USART1_RX_vect)
{
    register volatile uint8_t newByte = UDR1;

    // received data
    if (_usartRx[1]->_rxBuffer->write(newByte)) {
        _usartRx[1]->_rxDataReceivedSyn.signal();

    } else {
        _usartRx[1]->_rxOverflowSyn.signal();
    }
}


#endif


#ifdef UCSR2B


ISR(USART2_TX_vect)
{
    // last byte complete
    _usartTx[2]->byteTxComplete();
}


ISR(USART2_UDRE_vect)
{
    // need more data
    uint8_t nextByte;

    if (!_usartTx[2]->getNextTxByte(nextByte)) {
        UCSR2B &= ~(1 << UDRIE2);

    } else {
        UDR2 = nextByte;
    }
}


ISR(USART2_RX_vect)
{
    register volatile uint8_t newByte = UDR2;

    // received data
    if (_usartRx[2]->_rxBuffer->write(newByte)) {
        _usartRx[2]->_rxDataReceivedSyn.signal();

    } else {
        _usartRx[2]->_rxOverflowSyn.signal();
    }
}


#endif


#ifdef UCSR3B


ISR(USART3_TX_vect)
{
    // last byte complete
    _usartTx[3]->byteTxComplete();
}


ISR(USART3_UDRE_vect)
{
    // need more data
    uint8_t nextByte;

    if (!_usartTx[3]->getNextTxByte(nextByte)) {
        UCSR3B &= ~(1 << UDRIE3);

    } else {
        UDR3 = nextByte;
    }
}


ISR(USART3_RX_vect)
{
    register volatile uint8_t newByte = UDR3;

    // received data
    if (_usartRx[3]->_rxBuffer->write(newByte)) {
        _usartRx[3]->_rxDataReceivedSyn.signal();

    } else {
        _usartRx[3]->_rxOverflowSyn.signal();
    }
}


#endif


#endif
