//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_CLI_H
#define TCRI_ZERO_CLI_H


#include <stdint.h>

#include "thread.h"
#include "usart.h"
#include "cmdline.h"


typedef zero::UsartTx CliTx;
typedef zero::UsartRx CliRx;


namespace zero {

    class Shell : public Thread {
    public:
        // ctor
        Shell(
            const int usartNumber,
            const uint32_t baud );

    private:
        int main();

        void displayWelcome();
        void displayPrompt();
        void handleKeyboard();

        const int _usartNumber;
        const uint32_t _baud;
        CliTx* _tx;
        CliRx* _rx;
        CommandLine* _cmdLine;
    };

}    // namespace zero

#endif