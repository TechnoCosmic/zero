//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "doublebuffer.h"
#include "memory.h"


using namespace zero;


DoubleBuffer::DoubleBuffer(const uint16_t size) {
    if (_buffer = memory::allocate(size, &_bufferSize, memory::SearchStrategy::BottomUp)) {
        _pivot = _bufferSize / 2;
        _writeOffset = 0UL;
        _usedBytes = 0UL;
    }
}


DoubleBuffer::~DoubleBuffer() {
    memory::free(_buffer, _bufferSize);
    _buffer = 0UL;
    _bufferSize = 0UL;
}


bool DoubleBuffer::write(const uint8_t d) {
    bool rc = false;

    if (_usedBytes < _pivot) {
        _buffer[_writeOffset + _usedBytes] = d;
        _usedBytes++;
        rc = true;
    }

    return rc;
}


uint8_t* DoubleBuffer::getCurrentBuffer(uint16_t& numBytes) {
    const uint8_t oldSreg = SREG;
    cli();
    
    uint8_t* rc = 0UL;

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