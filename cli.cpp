/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"

#ifdef CLI_ENABLED

#include <stdint.h>
#include <avr/pgmspace.h>
#include "cli.h"
#include "thread.h"
#include "textpipe.h"
#include "iomanip.h"
#include "usart.h"
#include "string.h"

using namespace zero;


const char BELL = 7;
const char BACKSPACE = 8;
const char CR = 13;
const char ESCAPE = 27;


const PROGMEM char _cliRxPipeName[] = "cli_rx";
const PROGMEM char _cliTxPipeName[] = "cli_tx";


CliCommand::CliCommand(const char* name, const CliEntryPoint entry) {
    _entryPoint = entry;

    // system info
    _systemData._objectName = name;
    _systemData._objectType = CLICOMMAND;

    // add to the list of NamedObjects
    NamedObject::add((NamedObject*) this);
}


int CliCommand::execute(TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    return _entryPoint(rx, tx, argc, argv);
}


void displayPrompt(TextPipe* rx, TextPipe* tx) {
    *tx << green << "zero" << white << ": " << blue << "$ " << white;
}


static const PROGMEM char _welcomeText[] = "\fWelcome to zero";
static const PROGMEM char _cliOnUsart[] = "CLI on USART0 @ ";
static const PROGMEM char _bps[] = "bps";
static const PROGMEM char _speed[] = "MHz system";


void displayWelcome(TextPipe* rx, TextPipe* tx) {
    *tx << dec << PGM(_welcomeText) << zero::endl;
    *tx << PGM(_cliOnUsart) << (int32_t) CLI_BAUD << PGM(_bps) << zero::endl;
    *tx << (int) (F_CPU / 1000000UL) << PGM(_speed) << zero::endl;
}


int tokenize(char* s, char* argv[]) {
    int tokenCount = 0;
    bool lastWasSeparator = true;
    bool inQuotes = false;

    while (*s && tokenCount < CLI_CMD_LINE_MAX_TOKENS) {
        if (*s == '\"') {
            inQuotes = !inQuotes;
            *s = 0;
            lastWasSeparator = true;

        } else {
            if (inQuotes || *s != ' ') {
                if (lastWasSeparator) {
                    argv[tokenCount++] = s;
                }
                lastWasSeparator = false;

            } else if (*s == ' ') {
                *s = 0;
                lastWasSeparator = true;
            }
        }
        if (tokenCount-1 == 0) {
            *s = tolower(*s);
        }
        s++;
    }
    return tokenCount;
}


const PROGMEM char _exitedWithReturnCode[] = "\' exited with return code ";
const PROGMEM char _isNotCliCommand[] = "': is not a CLI command";
const PROGMEM char _cmdNotFound[] = "': command not found";

void processCommandLine(TextPipe* rx, TextPipe* tx, char* commandLine) {
    char* args[CLI_CMD_LINE_MAX_TOKENS];
    int count = tokenize(commandLine, args);

    if (count) {
        NamedObject* obj = NamedObject::find(args[0]);

        if (obj) {
            if (obj->_objectType != ZeroObjectType::CLICOMMAND) {
                *tx << '\'' << args[0] << PGM(_isNotCliCommand) << endl;

            } else {
                int returnCode = ((CliCommand*) obj)->execute(rx, tx, count, args);

                if (returnCode) {
                    *tx << '\'' << args[0] << PGM(_exitedWithReturnCode) << (int) returnCode << endl;
                }
            }
        } else {
            *tx << '\'' << args[0] << PGM(_cmdNotFound) << endl;
        }
    }
}


thread(cli, CLI_STACK_BYTES, 50, {
    TextPipe rx(_cliRxPipeName, CLI_RX_PIPE_BYTES);
    TextPipe tx(_cliTxPipeName, CLI_TX_PIPE_BYTES);
    Usart serial(CLI_BAUD, &rx, &tx);

    char cmdLine[CLI_CMD_LINE_BUFFER_BYTES];
    int16_t cursorPosition = 0L;

#ifndef CLI_VT100
    tx.setOutputType(OutputType::TEXT_ONLY);
#endif

    displayWelcome(&rx, &tx);
    displayPrompt(&rx, &tx);

    while (true) {
        bool echo = true;
        uint8_t input = 0;

        // blocking call to read() to gather keystrokes from the USART
        if (rx.read(&input, true)) {
            switch (input) {
                case ESCAPE:
                    // clear the line and start fresh
                    if (cursorPosition > 0 && tx.getOutputType() == OutputType::VT100) {
                        // clear command line
                        tx << "\e[2K\r";

                        // begin again
                        memset((uint8_t*) cmdLine, 0, sizeof(cmdLine));
                        cursorPosition = 0;
                        displayPrompt(&rx, &tx);
                    }
                    echo = false;
                break;

                case BACKSPACE:
                    echo = false;
                    if (cursorPosition > 0) {
                        cursorPosition--;
                        cmdLine[cursorPosition] = 0;

                        // backspace + clear to end of line

                        // TODO: Make these PROGMEM when iomanip.h
                        // is written
                        tx << "\010\e[K";
                    } else {
                        tx << (char) BELL;
                    }
                break;

                case CR:
                    echo = false;
                    tx << endl;

                    // send the command line off for processing
                    processCommandLine(&rx, &tx, cmdLine);

                    // begin again
                    memset((uint8_t*) cmdLine, 0, sizeof(cmdLine));
                    cursorPosition = 0;
                    displayPrompt(&rx, &tx);
                break;

                default:
                    // Check for buffer underrun and overflow - safety first, CASE!
                    if (cursorPosition >= 0 && cursorPosition < CLI_CMD_LINE_BUFFER_BYTES) {
                        cmdLine[cursorPosition++] = input;

                    } else {
                        echo = false;
                        tx << (char) BELL;
                    }
                break;
            }

            if (echo) {
                tx << (char) input;
            }
        }
    }

    return 0;
});


clicommand(clear, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    displayWelcome(rx, tx);
    return 0;
});

#endif
