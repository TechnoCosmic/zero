//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_PIPE


#include <util/atomic.h>

#include "pipe.h"
#include "memory.h"
#include "thread.h"
#include "synapse.h"


using namespace zero;


// ctor
Pipe::Pipe(const uint16_t size)
{
    // storage and tracking
    _buffer = (uint8_t*) memory::allocate( size, &_bufferSize );
    _startIndex = _length = 0UL;
}


// dtor
Pipe::~Pipe()
{
    memory::free( _buffer, _bufferSize );

    if (_dataAvailSyn) {
        _dataAvailSyn->clearSignals();
    }

    if (_roomAvailSyn) {
        _roomAvailSyn->clearSignals();
    }
}


// validity checking
Pipe::operator bool() const
{
    return _buffer != nullptr;
}


// Returns true if the Pipe is empty, false otherwise
bool Pipe::isEmpty() const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        return _length == 0;
    }
}


// Returns true if the Pipe is full, false otherwise
bool Pipe::isFull() const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        return _length == _bufferSize;
    }
}


// Reads a byte from the Pipe. Returns true if data was
// successfully read, false otherwise.
bool Pipe::read(uint8_t& data)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bool rc = false;

        while (isEmpty() && _dataAvailSyn) {
            _dataAvailSyn->wait();
            cli();
        }

        // select the byte and invoke the filter
        bool doIt = true;
        uint8_t dataToRead = _buffer[_startIndex];

        if (_readFilter) {
            doIt = _readFilter( dataToRead );
        }

        if (doIt) {
            data = dataToRead;
            _startIndex++;
            _length--;

            if (_startIndex == _bufferSize) {
                _startIndex = 0UL;
            }

            if (_roomAvailSyn) {
                _roomAvailSyn->signal();
            }

            rc = true;
        }

        return rc;
    }
}


// Writes a byte to the Pipe. Returns true if data was
// successfully written, false otherwise.
bool Pipe::write(const uint8_t data)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bool rc = false;

        while (isFull() && _roomAvailSyn) {
            _roomAvailSyn->wait();
            cli();
        }
        
        uint16_t index = _startIndex + _length;

        if (index >= _bufferSize) {
            index -= _bufferSize;
        }
        
        // invoke the filter
        bool doIt = true;
        uint8_t dataToWrite = data;

        if (_writeFilter) {
            doIt = _writeFilter( dataToWrite );
        }

        if (doIt) {
            _buffer[index] = dataToWrite;
            _length++;

            if (_dataAvailSyn) {
                _dataAvailSyn->signal();
            }

            rc = true;
        }

        return rc;
    }
}


// Empties the Pipe
void Pipe::flush()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _startIndex = _length = 0UL;

        if (_dataAvailSyn) {
            _dataAvailSyn->clearSignals();
        }

        if (_roomAvailSyn) {
            _roomAvailSyn->signal();
        }
    }
}


// Assigns read filter to the Pipe
void Pipe::setReadFilter(PipeFilter f)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _readFilter = f;
    }
}


// Assigns write filter to the Pipe
void Pipe::setWriteFilter(PipeFilter f)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _writeFilter = f;
    }
}


// Assigns a Synapse to signal when the Pipe has
// room to accept more data.
void Pipe::setRoomAvailSynapse(Synapse& s)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _roomAvailSyn = &s;

        if (!isFull()) {
            _roomAvailSyn->signal();
        }
        else {
            _roomAvailSyn->clearSignals();
        }
    }
}


// Assigns a Synapse to signal when the Pipe has
// data waiting to be read.
void Pipe::setDataAvailSynapse(Synapse& s)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _dataAvailSyn = &s;

        if (!isEmpty()) {
            _dataAvailSyn->signal();
        }
        else {
            _dataAvailSyn->clearSignals();
        }
    }
}


#endif
