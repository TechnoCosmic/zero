/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"
#include "memory.h"
#include "cli.h"

using namespace zero;


// Simple byte search tracking class
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

        // if we're at the end of the match string,
        // see if we matched
        if (nextChar == 0) {
            rc = (_cursor != 0);
            reset();
        }

        return rc;
    }

    uint16_t _address;
    memory::MemoryType _source;
    uint8_t _cursor;
    uint8_t _lastMatchIndex;

};


#define NEW_CURSOR_UP 0xEF
#define NEW_CURSOR_DOWN 0xEE

const PROGMEM char _curUpEsc[] = "\e[A";
const PROGMEM char _curDownEsc[] = "\e[B";
SearchCapture _curUp(_curUpEsc, memory::MemoryType::FLASH);
SearchCapture _curDown(_curDownEsc, memory::MemoryType::FLASH);

bool cliRxEscapeFilter(Pipe* p, uint8_t* data) {
    const bool captureMadeUp = _curUp.notifyCharacter(*data);

    if (captureMadeUp) {
        *data = NEW_CURSOR_UP;
    }

    const bool captureMadeDown = _curDown.notifyCharacter(*data);

    if (captureMadeDown) {
        *data = 27;
    }

    return true;
}
