/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include "usart.h"
#include "pipe.h"
#include "thread.h"


using namespace zero;

#ifndef USART_RX_vect
#define USART_RX_vect USART0_RX_vect
#endif

#ifndef USART_UDRE_vect
#define USART_UDRE_vect USART0_UDRE_vect
#endif

static Usart* _usart = 0UL;


bool writeFilter(uint8_t* data) {
    UCSR0B |= (1 << UDRIE0);
    return true;
}


Usart::Usart(const uint32_t baud, Pipe* rx, Pipe* tx) {
    // set up the USART hardware
    const uint16_t pre = (F_CPU / (16UL * baud)) - 1;


    power_usart0_enable();

    _rx = rx;
    _tx = tx;

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
        if (_rx) {
            // setup for receiving data
            UCSR0B |= (1 << RXEN0) | (1 << RXCIE0);
        }
        if (_tx) {
            // setup for transmitting data
            UCSR0B |= (1 << TXEN0) | (1 << UDRIE0);
    
            // hook into the transmit Pipe's write() filter so we
            // can enable and disable the USART TX ISR when needed
            _tx->setWriteFilter(writeFilter);
        }
    
        // 8-none-1
        UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
    
        // speed
        UBRR0H = (uint8_t) pre >> 8;
        UBRR0L = (uint8_t) pre;
    
        // remember who we are for the ISRs
        _usart = this;
    }
}

Pipe* Usart::getRxPipe() {
    return _rx;
}

Pipe* Usart::getTxPipe() {
    return _tx;
}

ISR(USART_RX_vect) {
    cli();

    // write the incoming byte to the Pipe
    if (!_usart->getRxPipe()->write(UDR0, false)) {
        // TODO: Error - buffer full
    }
}

ISR(USART_UDRE_vect) {
    cli();

    Pipe* tx = _usart->getTxPipe();

    if (tx->isEmpty()) {
        UCSR0B &= ~(1 << UDRIE0);

    } else {
        uint8_t data;
        tx->read(&data, false);
        UDR0 = data; 
    }
}

Usart& operator<<(Usart& out, const char c) {
	out.getTxPipe()->write(c, true);
	return out;
}

Usart& operator<<(Usart& out, const char* s) {
	out.getTxPipe()->write(s);
	return out;
}
