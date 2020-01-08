//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "thread.h"
#include "memory.h"
#include "debug.h"
#include "usart.h"
#include "suart.h"
#include "util.h"

#include "sram.h"


using namespace zero;


// flash an LED on C0
int resetIndicatorThread()
{
    for (auto i = 0; i < 20; i++) {
        if (PINC & (1 << 4)) {
            PORTC ^= (1 << 0);

        } else {
            PORTC &= ~(1 << 0);
        }

        me.wait(0, 175);
    }
}


// flash an LED on C2
int limitedFlasherThread()
{
    for (auto i = 0; i < 30; i++) {
        if (PINC & (1 << 6)) {
            PORTC ^= (1 << 2);

        } else {
            PORTC &= ~(1 << 2);
        }

        _delay_ms(75);
    }

    return 0xAA;
}


int serialStreamer()
{
    auto buffer = (char*) memory::allocate(97, 0UL, memory::SearchStrategy::BottomUp);

    // fill the buffer
    auto cur = buffer;

    for (auto i = 32; i < 127; i++) {
        *cur++ = i;
    }

    // end the buffer with CR/LF
    *cur++ = '\r';
    *cur++ = '\n';
    *cur++ = 0;

    // get the hardware USARTs configured
    auto htx0 = new UsartTx(0);
    auto hardTx0ReadySig = me.allocateSignal();

    htx0->setCommsParams(9600);
    htx0->enable(hardTx0ReadySig);

    // get the software USART configured
    auto stx = new SuartTx();
    auto softTxReadySig = me.allocateSignal();

    stx->setCommsParams(9600, &DDRA, &PORTA, 0);
    stx->enable(softTxReadySig);

    // tracking
    uint8_t lineNum = 0;

    // the main loop
    while (true) {
        auto wokeSigs = me.wait(hardTx0ReadySig | softTxReadySig);

        if (wokeSigs & hardTx0ReadySig) {
            htx0->transmit(buffer, strlen(buffer));
        }

        if (wokeSigs & softTxReadySig) {
            // swap TX pins
            stx->setCommsParams(9600, &DDRA, &PORTA, lineNum & 0b1);
            stx->enable(softTxReadySig);

            // send the buffer to the new TX pin
            stx->transmit(buffer, strlen(buffer));
            lineNum += 1;
        }
    }
}


// // This function is required by the kernel. It is run whenever there's nothing
// // else to do. For now, this is a user-definable function, for debugging purposes.
// int idleThreadEntry()
// {
//     while (true);
// }


int rxTimeoutTest()
{
    auto rxDataSig = me.allocateSignal();
    auto rxOvfSig = me.allocateSignal();
    auto txReadySig = me.allocateSignal();

    auto rx = new UsartRx(1);
    rx->setCommsParams(9600);
    rx->enable(128, rxDataSig, rxOvfSig);

    auto tx = new UsartTx(1);
    tx->setCommsParams(9600);
    tx->enable(txReadySig);

    me.wait(txReadySig);
    tx->transmit("\frx Timeout Test\r\n", 18);

    me.wait(txReadySig);
    tx->transmit("rx Timeout Test\r\n", 17);

    while (true) {
        auto wokeSigs = me.wait(rxDataSig | rxOvfSig, 2000);

        if (wokeSigs & SIG_TIMEOUT) {
            me.wait(txReadySig);
            tx->transmit("*** TIMEOUT\r\n", 13);
        }

        if (wokeSigs & rxDataSig) {
            uint16_t numBytes;

            while (auto buffer = rx->getCurrentBuffer(numBytes)) {
                me.wait(txReadySig);
                tx->transmit(buffer, numBytes);
            }
        }

        if (wokeSigs & rxOvfSig) {
            me.wait(txReadySig);
            tx->transmit("*** OVF\r\n", 9);
        }
    }
}


int delayTest()
{
    DDRC |= 4;

    while (true) {
        PORTC ^= 4;
        me.wait(0UL, 290);
    }
}


int delayTest2()
{
    DDRC |= 8;

    while (true) {
        PORTC ^= 8;
        me.wait(0UL, 300);
    }
}


int spiTest()
{
    auto spiReadySig = me.allocateSignal();
    auto ctrl = new SpiMemory(&DDRB, &PORTB, PINB4, spiReadySig);
    auto inBuf = (char*) memory::allocate(128, 0UL, memory::SearchStrategy::BottomUp);

    // clear the screen
    dbg('\f');

    // asynchronous write
    me.wait(spiReadySig);
    ctrl->write((char*) "Beaker and Sookie are my babies!\r\n", 0ULL, 34);
    
    // asynchronous write again
    me.wait(spiReadySig);
    ctrl->write((char*) "I love them very much!\r\n", 65536ULL, 24);
    
    // asynchronous xfer back again, into a new buffer
    me.wait(spiReadySig);
    ctrl->read(inBuf, 0ULL, 34);

    // wait for that to finish
    me.wait(spiReadySig);

    // diplay it
    for (auto i = 0; i < 34; i++) {
        char c = inBuf[i];
        dbg(c);
    }

    // asynchronous xfer back again, into a new buffer
    ctrl->read(inBuf, 65536ULL, 24);

    // wait for that to finish
    me.wait(spiReadySig);

    // diplay it
    for (auto i = 0; i < 24; i++) {
        char c = inBuf[i];
        dbg(c);
    }

    // free the in buffer
    memory::free(inBuf, 128);
    inBuf = 0UL;
}


// This function is required by the kernel. It is run whenever there is nothing
// else to do. For now, this is a user-definable function, for debugging purposes,
// such as flashing an LED to show when the MCU is idle.
// NOTE: Do NOT block in the idle thread. Always be busy, or put the MCU to sleep.
int idleThreadEntry()
{
    DDRC |= 2;

    while (true) {
        PORTC ^= 2;
        _delay_ms(750);
    }
}


// This function is the main entry point for all zero programs. Set things up, spawn
// your initial Threads. Get your party started here.
void startup_sequence()
{
    // // GPIO setup for LEDs and buttons
    // DDRC = 0b00001111;
    // PORTC = 0b11110000;

    new Thread(128, resetIndicatorThread, TF_FIRE_AND_FORGET);
    // new Thread(128, serialStreamer, TF_FIRE_AND_FORGET);
    // new Thread(128, rxTimeoutTest, TF_FIRE_AND_FORGET);
    // new Thread(128, delayTest, TF_FIRE_AND_FORGET);
    // new Thread(128, delayTest2, TF_FIRE_AND_FORGET);

    new Thread(128, spiTest, TF_FIRE_AND_FORGET);
}

// void startup_sequence() {}
// int idleThreadEntry() { while(1); }