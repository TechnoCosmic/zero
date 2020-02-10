//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <string.h>
#include <avr/pgmspace.h>

#include "cli.h"
#include "thread.h"
#include "synapse.h"
#include "usart.h"

#include "cmdline.h"


using namespace zero;


namespace {
    const auto WELCOME = "\fWelcome to zero\r\n";
    const auto USART = "CLI on USART";
    const auto PROMPT = "zero: $ ";
    const auto CRLF = "\r\n";

    const auto CLI_STACK_BYTES = 512;
    const auto CLI_RX_BYTES = 32;
    const auto CLI_CMD_BYTES = 80;
}


// ctor
Shell::Shell(
    const int usartNumber,
    const uint32_t baud)
:
    // call parent ctor, with entryPoint as a lambda.
    // This is a stub that just calls ::main()
    Thread( PSTR( "cli" ), CLI_STACK_BYTES, []()
    {
        return ((Shell&) me).main();
    }),

    // other params
    _usartNumber{ usartNumber },
    _baud{ baud },
    _tx{ nullptr },
    _rx{ nullptr },
    _cmdLine{ nullptr }
{
    // ctor body
}


void Shell::displayWelcome()
{
    _tx->transmit( WELCOME, strlen(WELCOME), true );
    _tx->transmit( USART, strlen(USART), true );

    char usartAscii = '0' + _usartNumber;

    _tx->transmit( &usartAscii, 1, true );
    _tx->transmit( "\r\n", 2, true );
}


void Shell::displayPrompt()
{
    _tx->transmit( PROMPT, strlen(PROMPT), true );
}


void Shell::handleKeyboard()
{
    char echoChar = 0;
    uint16_t numBytes;

    while(auto buffer = _rx->getCurrentBuffer( numBytes )) {
        for (uint16_t i = 0; i < numBytes; i++) {
            switch (auto curChar = buffer[i]) {
                case '\r':
                    _cmdLine->process();
                    _cmdLine->clear();
                    _tx->transmit( CRLF, 2, true );
                    displayPrompt();
                break;

                default:
                    if (_cmdLine->registerKeyPress( curChar )) {
                        echoChar = curChar;
                    }
                break;
            }

            if (echoChar) {
                _tx->transmit( &echoChar, 1, true );
                echoChar = 0;
            }
        }
    }

}


int Shell::main()
{
    // set up the receiver
    Synapse rxDataSyn;
    CliRx rx( _usartNumber );

    if (!rxDataSyn || !rx) {
        return 20;
    }

    rx.setCommsParams( _baud );
    rx.enable( CLI_RX_BYTES, rxDataSyn, 0UL );

    // set up the transmitter
    Synapse txReadySyn;
    CliTx tx( _usartNumber );
    
    if (!txReadySyn || !tx) {
        return 20;
    }

    tx.setCommsParams( _baud );
    tx.enable( txReadySyn );

    // command line storage
    CommandLine cmdLine( CLI_CMD_BYTES );
    
    if (!cmdLine) {
        return 20;
    }
    
    // remember these things now that we're set up
    _tx = &tx;
    _rx = &rx;
    _cmdLine = &cmdLine;

    // hello!
    displayWelcome();
    displayPrompt();

    // main loop
    while (true) {
        rxDataSyn.wait();
        handleKeyboard();
    }
}
