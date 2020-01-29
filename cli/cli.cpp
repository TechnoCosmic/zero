//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <string.h>

#include "cli.h"
#include "thread.h"
#include "synapse.h"
#include "memory.h"
#include "usart.h"

#include "cmdline.h"


using namespace zero;


namespace {
    const auto WELCOME = "\fWelcome to zero\r\n";
    const auto PROMPT = "zero: $ ";
    const auto CRLF = "\r\n";

    const auto CLI_USART_NUM = 0;
    const auto CLI_BAUD = 9600;
    const auto CLI_RX_BYTES = 32;
    const auto CLI_CMD_BYTES = 80;
}


void displayWelcome(CliTx& tx)
{
    tx.transmit(WELCOME, strlen(WELCOME), true);
}


void displayPrompt(CliTx& tx)
{
    tx.transmit(PROMPT, strlen(PROMPT), true);
}


void handleKeyboard(CliRx& rx, CliTx& tx, CommandLine& cmdLine)
{
    char echoChar = 0;
    uint16_t numBytes;

    while(auto buffer = rx.getCurrentBuffer(numBytes)) {
        for (uint16_t i = 0; i < numBytes; i++) {
            switch (auto curChar = buffer[i]) {
                case '\r':
                    cmdLine.process(rx, tx);
                    cmdLine.clear();
                    tx.transmit(CRLF, 2, true);
                    displayPrompt(tx);
                break;

                default:
                    if (cmdLine.registerKeyPress(curChar)) {
                        echoChar = curChar;
                    }
                break;
            }

            if (echoChar) {
                tx.transmit(&echoChar, 1, true);
                echoChar = 0;
            }
        }
    }

}


int cliEntry()
{
    // set up the receiver
    Synapse rxDataSyn;
    CliRx rx(CLI_USART_NUM);
    if (!rxDataSyn || !rx) return 20;

    rx.setCommsParams(CLI_BAUD);
    rx.enable(CLI_RX_BYTES, rxDataSyn, 0UL);

    // set up the transmitter
    Synapse txReadySyn;
    UsartTx tx(CLI_USART_NUM);
    if (!txReadySyn || !tx) return 20;

    tx.setCommsParams(CLI_BAUD);
    tx.enable(txReadySyn);

    // command line storage
    CommandLine cmdLine(CLI_CMD_BYTES);
    if (!cmdLine) return 20;

    // hello!
    displayWelcome(tx);
    displayPrompt(tx);

    // main loop
    while (true) {
        auto wokeSigs = me.wait(rxDataSyn);

        if (wokeSigs & rxDataSyn) {
            handleKeyboard(rx, tx, cmdLine);
        }
    }
}
