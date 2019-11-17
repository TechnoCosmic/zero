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


static const char BELL = 7;
static const char BACKSPACE = 8;
static const char TAB = 9;
static const char CR = 13;
static const char ESCAPE = 27;

static const PROGMEM char _cliRxPipeName[] = "cli_rx";
static const PROGMEM char _cliTxPipeName[] = "cli_tx";


CliCommand::CliCommand(const char* name, const CliEntryPoint entry) {
    _entryPoint = entry;

    // system info
    _systemData._objectName = name;
    _systemData._objectType = CLICOMMAND;

    // add to the list of NamedObjects
    NamedObject::add((NamedObject*) this);
}


void CliCommand::execute(const char* commandLine) {
    Pipe* rx = (Pipe*) Pipe::find("cli_rx");

    if (rx) {
        *((TextPipe*) rx) << commandLine;
    }
}


int CliCommand::execute(TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    return _entryPoint(rx, tx, argc, argv);
}


void displayPrompt(TextPipe* rx, TextPipe* tx) {
    *tx << green << PROJ_NAME << white << ": " << blue << "$ " << white;
}


static const PROGMEM char _welcomeText[] = "\fWelcome to " PROJ_NAME ", powered by zero ";
static const PROGMEM char _cliOnUsart[] = "CLI on USART0 @ ";
static const PROGMEM char _bps[] = "bps";
static const PROGMEM char _speed[] = "MHz system";


// displays the 'startup' welcome message
void displayWelcome(TextPipe* rx, TextPipe* tx) {
    *tx << dec << PGM(_welcomeText) << 'v' << ZERO_BUILD_VERSION << '.' << ZERO_BUILD_REVISION << endl;
    *tx << PGM(_cliOnUsart) << (int32_t) CLI_BAUD << PGM(_bps) << endl;
    *tx << (int) (F_CPU / 1000000UL) << PGM(_speed) << endl;
}


// Turns a single command line string into individual arguments.
// This happens in-place, turning whitespace (that is outside of
// quoation marks) into nulls (\0), and setting up pointers to the
// start of each argument as it goes.
// Also, the first 'word' in the command line is forced lowercase.
uint8_t tokenize(char* s, char* argv[]) {
    uint8_t tokenCount = 0;
    bool lastWasSeparator = true;
    bool inQuotes = false;

    while (*s && tokenCount < CLI_CMD_LINE_MAX_TOKENS) {
        if (*s == '\"') {
            inQuotes = !inQuotes;
            *s = 0;
            lastWasSeparator = true;

        } else {
            if (inQuotes || !isspace(*s)) {
                if (lastWasSeparator) {
                    argv[tokenCount++] = s;
                }

                lastWasSeparator = false;

            } else if (isspace(*s)) {
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


static const PROGMEM char _exitedWithReturnCode[] = "\' exited with return code ";
static const PROGMEM char _isNotCliCommand[] = "': is not a CLI command";
static const PROGMEM char _cmdNotFound[] = "': command not found";


// Tokenizes and executes the given command line
void processCommandLine(TextPipe* rx, TextPipe* tx, char* commandLine) {
    char* args[CLI_CMD_LINE_MAX_TOKENS];
    uint8_t count = tokenize(commandLine, args);

    if (count) {
        NamedObject* obj = NamedObject::find(args[0], ZeroObjectType::CLICOMMAND);

        if (obj) {
            int returnCode = ((CliCommand*) obj)->execute(rx, tx, count, args);

            if (returnCode) {
                *tx << '\'' << args[0] << PGM(_exitedWithReturnCode) << (int32_t) returnCode << endl;
            }
        } else {
            *tx << '\'' << args[0] << PGM(_cmdNotFound) << endl;
        }
    }
}


void handleTabCompletion(TextPipe* cliInputPipe, char* commandLine, const int16_t cursorPosition) {
    int16_t pos = cursorPosition;

    // step to the previous whitespace
    while (--pos) {
        if (commandLine[pos] == ' ') {
            pos++;
            break;
        }
    }

    uint16_t typedLength = strlen(&commandLine[pos]) - 1;

    if (pos >= 0) {
        NamedObject* obj = NamedObject::findFirstByPattern(&commandLine[pos]);

        if (obj) {
            const uint16_t spaceNeeded = strlenpgm(obj->_objectName);

            // copy the object's name into the command line
            for (uint16_t i = typedLength; i < spaceNeeded; i++) {
                const char inChar = pgm_read_byte(obj->_objectName + i);

                // inject characters into the CLI input Pipe
                *cliInputPipe << (char) inChar;
            }

            // add a trailing space for kindness
            *cliInputPipe << ' ';
        }
    }
}


class SearchCapture {
public:

    SearchCapture(const char* string, memory::MemoryType source) {
        _address = (uint16_t) string;
        _source = source;
        _cursor = 0;
        _lastMatchIndex = -1;
    }

    void reset() {
        _cursor = 0;
        _lastMatchIndex = 1;
    }

    bool notifyCharacter(const char c) {
        bool rc = false;
        const char curChar =  memory::read((void*) (_address + _cursor), _source);
        const char nextChar =  memory::read((void*) (_address + _cursor + 1), _source);

        if (curChar == c) {
            _cursor++;

        } else {
            _lastMatchIndex = _cursor - 1;
            _cursor = 0;
        }

        if (nextChar == 0) {
            rc = (_cursor != 0);

            if (rc) {
                reset();
            }
        }

        return rc;
    }

    uint16_t _address;
    memory::MemoryType _source;
    uint8_t _cursor;
    uint8_t _lastMatchIndex;

};


static const PROGMEM char _curUpEsc[] = "\e[A";
static const PROGMEM char _bsCoelEsc[] = "\010\e[K";
static const PROGMEM char _clCrEsc[] = "\e[2K\r";


// The main entry point for the CLI Thread
int cliMain() {
    TextPipe rx(_cliRxPipeName, CLI_RX_PIPE_BYTES);
    TextPipe tx(_cliTxPipeName, CLI_TX_PIPE_BYTES);
    Usart serial(CLI_BAUD, &rx, &tx);

    char prevLine[CLI_CMD_LINE_BUFFER_BYTES];
    char cmdLine[CLI_CMD_LINE_BUFFER_BYTES];
    int16_t cursorPosition = 0L;
    SearchCapture _curUp(_curUpEsc, memory::MemoryType::FLASH);

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
            if (tx.getOutputType() == VT100 && _curUp.notifyCharacter(input)) {
                // clear the current line and re-display the prompt
                tx << PGM(_clCrEsc);
                displayPrompt(&rx, &tx);

                // restore the history to the command line
                memcpy((uint8_t*) cmdLine, (uint8_t*) prevLine, CLI_CMD_LINE_BUFFER_BYTES);
                cursorPosition = strlen(cmdLine);

                // output the command line to the console
                tx << cmdLine;

                echo = false;
                continue;
            }

            switch (input) {
                case TAB:
                    cmdLine[cursorPosition] = '*';
                    handleTabCompletion(&rx, cmdLine, cursorPosition);
                    cmdLine[cursorPosition] = 0;
                    echo = false;
                break;

                case ESCAPE:
                    // clear the line and start fresh
                    if (cursorPosition > 0 && tx.getOutputType() == OutputType::VT100) {
                        // clear command line
                        tx << PGM(_clCrEsc);

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
                        tx << PGM(_bsCoelEsc);

                    } else {
                        tx << (char) BELL;
                    }
                break;

                case CR:
                    echo = false;
                    tx << endl;

                    // preserve the command line in the history, before
                    // it gets in-place tokenized!
                    memcpy((uint8_t*) prevLine, (uint8_t*) cmdLine, CLI_CMD_LINE_BUFFER_BYTES);

                    // send the command line off for processing
                    processCommandLine(&rx, &tx, cmdLine);

                    // begin again
                    memset((uint8_t*) cmdLine, 0, sizeof(cmdLine));
                    cursorPosition = 0;
                    displayPrompt(&rx, &tx);
                break;

                default:
                    // Check for buffer underrun and overflow - safety first, CASE!
                    if (cursorPosition >= 0 && cursorPosition < (CLI_CMD_LINE_BUFFER_BYTES - 1)) {
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
}


clicommand(clear, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    displayWelcome(rx, tx);
    return 0;
});

#endif
