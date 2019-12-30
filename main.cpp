//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <stdint.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "thread.h"
#include "memory.h"
#include "usart.h"
#include "suart.h"


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

        _delay_ms(175);
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


int usartEchoDemo()
{
    SignalField txCompleteSig = me.allocateSignal();
    UsartTx* tx = new UsartTx(1);
    tx->setCommsParams(9600);
    tx->enable(txCompleteSig);

    SignalField rxSig = me.allocateSignal();
    SignalField rxOvfSig = me.allocateSignal();
    UsartRx* rx = new UsartRx(1);
    rx->setCommsParams(9600);
    rx->enable(96, rxSig, rxOvfSig);

    // Say hello!
    tx->transmit("\fzero USART blocking echo test\r\n", 32);

    // main loop
    while(true) {
        SignalField wokeSigs = me.wait(rxSig | rxOvfSig);

        // check to see if we woke for new USART data
        if (wokeSigs & rxSig) {
            uint16_t numBytes;

            while (uint8_t* rxData = rx->getCurrentBuffer(numBytes)) {
                me.wait(txCompleteSig);
                tx->transmit(rxData, numBytes);
            }
        }

        // or if we woke because the receive buffer has overflowed
        if (wokeSigs & rxOvfSig) {
            me.wait(txCompleteSig);
            tx->transmit("*** BUFFER FULL ***\r\n", 21);
        }
    }
}


int serialStreamer()
{
    char* buffer = (char*) memory::allocate(97, 0UL, memory::SearchStrategy::BottomUp);

    // fill the buffer
    char* cur = buffer;

    for (auto i = 32; i < 127; i++) {
        *cur++ = i;
    }

    // end the buffer with CR/LF
    *cur++ = '\r';
    *cur++ = '\n';
    *cur++ = 0;

    // get the hardware USARTs configured
    UsartTx* htx0 = new UsartTx(0);
    SignalField hardTx0CompleteSig = me.allocateSignal();

    htx0->setCommsParams(9600);
    htx0->enable(hardTx0CompleteSig);

    // get the software USART configured
    SuartTx* stx = new SuartTx();
    SignalField softTxCompleteSig = me.allocateSignal();

    stx->setCommsParams(9600, &DDRA, &PORTA, 0);
    stx->enable(softTxCompleteSig);

    // clear the screen
    uint8_t cls = '\f';
    htx0->transmit(&cls, 1);
    stx->transmit(&cls, 1); 

    // tracking
    uint8_t lineNum = 0;

    // the main loop
    while (true) {
        SignalField wokeSigs = me.wait(hardTx0CompleteSig | softTxCompleteSig);

        if (wokeSigs & hardTx0CompleteSig) {
            *htx0 << buffer;
        }

        if (wokeSigs & softTxCompleteSig) {
            // swap TX pins
            stx->setCommsParams(9600, &DDRA, &PORTA, lineNum & 0b1);
            stx->enable(softTxCompleteSig);

            // send the buffer to the new TX pin
            *stx << buffer;
            lineNum += 1;
        }
    }
}


// This function is required by the kernel. It is run whenever there is nothing
// else to do. For now, this is a user-definable function, for debugging purposes.
int idleThreadEntry()
{
    DDRC |= (1 << 3);

    while (true) {
        PORTC ^= (1 << 3);
        _delay_ms(200);
    }
}


// // This function is required by the kernel. It is run whenever there's nothing
// // else to do. For now, this is a user-definable function, for debugging purposes.
// int idleThreadEntry()
// {
//     while (true);
// }


// This function is the main entry point for all zero programs. Set things up, spawn
// your initial Threads. Get your party started here.
void startup_sequence()
{
    // GPIO setup for LEDs and buttons
    DDRC = 0b00001111;
    PORTC = 0b11110000;

    
    // create the Thread objects
    // new Thread(128, limitedFlasherThread, TF_FIRE_AND_FORGET);
    new Thread(128, resetIndicatorThread, TF_FIRE_AND_FORGET);
    new Thread(128, serialStreamer, TF_FIRE_AND_FORGET);
    new Thread(128, usartEchoDemo, TF_FIRE_AND_FORGET);
}
