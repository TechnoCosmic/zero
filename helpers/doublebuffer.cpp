//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "doublebuffer.h"
#include "memory.h"


using namespace zero;


// ctor
DoubleBuffer::DoubleBuffer(const uint16_t size)
{
    if ((_buffer = (uint8_t*) memory::allocate(size, &_bufferSize))) {
        _pivot = _bufferSize / 2;
        _writeOffset = 0UL;
        _usedBytes = 0UL;
    }
}


// dtor
DoubleBuffer::~DoubleBuffer()
{
    memory::free(_buffer, _bufferSize);
}


// Writes a byte to the active buffer.
bool DoubleBuffer::write(const uint8_t d)
{
    bool rc = false;
    const uint8_t oldSreg = SREG;
    cli();
    
    if (_usedBytes < _pivot) {
        _buffer[_writeOffset + _usedBytes] = d;
        _usedBytes++;

        rc = true;
    }

    SREG = oldSreg;

    return rc;
}


// Returns the currently active half of the buffer (if
// there's any data in it), and swaps buffers.
uint8_t* DoubleBuffer::getCurrentBuffer(uint16_t& numBytes)
{
    const uint8_t oldSreg = SREG;
    cli();
    
    uint8_t* rc = nullptr;

    // tell the caller how many bytes we have
    numBytes = _usedBytes;

    // clear ready for more data
    _usedBytes = 0;

    // return the current half of the buffer
    if (numBytes) {
        rc = &_buffer[_writeOffset];
    }

    // swap to the other half
    _writeOffset = _writeOffset == 0 ? _pivot : 0;

    // restore SREG and escape        
    SREG = oldSreg;

    return rc;
}


// Clears the buffer
void DoubleBuffer::flush()
{
    const uint8_t oldSreg = SREG;
    cli();
        
    _writeOffset = 0UL;
    _usedBytes = 0UL;
        
    SREG = oldSreg;
}
