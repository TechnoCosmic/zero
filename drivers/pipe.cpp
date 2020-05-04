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


using namespace zero;


/// @brief Creates a new Pipe of a given size
/// @param size The size, in bytes, of the Pipe's buffer
Pipe::Pipe( const uint16_t size )
:
    _buffer{ (uint8_t*) memory::allocate( size, &_bufferSize ) }
{
    if ( *this ) {
        flush();
    }
}


// dtor
Pipe::~Pipe()
{
    memory::free( _buffer, _bufferSize );

    if ( _dataAvailSyn ) {
        _dataAvailSyn->clearSignals();
    }

    if ( _roomAvailSyn ) {
        _roomAvailSyn->clearSignals();
    }
}


/// @brief Determines if the Pipe initialized correctly
/// @returns `true` if the Pipe initialized correctly, `false` otherwise.
Pipe::operator bool() const
{
    return _buffer;
}


/// @brief Determines if the Pipe is empty
/// @returns `true` if the Pipe is empty, `false` otherwise.
bool Pipe::isEmpty() const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        return _length == 0;
    }
}


/// @brief Determines if the Pipe is full
/// @returns `true` if the Pipe is full, `false` otherwise.
bool Pipe::isFull() const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        return _length == _bufferSize;
    }
}


/// @brief Reads a byte from the Pipe
/// @param data A reference to the place to store the byte from the Pipe.
/// @returns `true` if the data was successfully read (`data` will be
/// valid), `false` otherwise.
bool Pipe::read( uint8_t& data )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        bool rc{ false };

        while ( isEmpty() and _dataAvailSyn ) {
            _dataAvailSyn->wait();
            cli();
        }

        // select the byte and invoke the filter
        bool doIt{ true };
        uint8_t dataToRead{ _buffer[ _startIndex ] };

        if ( _readFilter ) {
            doIt = _readFilter( dataToRead );
        }

        if ( doIt ) {
            data = dataToRead;
            _startIndex++;
            _length--;

            if ( _startIndex == _bufferSize ) {
                _startIndex = 0;
            }

            if ( _roomAvailSyn ) {
                _roomAvailSyn->signal();
            }

            rc = true;
        }

        return rc;
    }
}


/// @brief Writes a byte to the Pipe
/// @param data The byte to write to the Pipe.
/// @returns `true` if the data was successfully written, `false` otherwise.
bool Pipe::write( const uint8_t data )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        bool rc{ false };

        while ( isFull() and _roomAvailSyn ) {
            _roomAvailSyn->wait();
            cli();
        }

        // invoke the filter
        bool doIt{ true };
        uint8_t dataToWrite{ data };

        if ( _writeFilter ) {
            doIt = _writeFilter( dataToWrite );
        }

        if ( doIt ) {
            // work out where to put the incoming byte
            uint16_t index { _startIndex + _length };

            if ( index >= _bufferSize ) {
                index -= _bufferSize;
            }

            _buffer[ index ] = dataToWrite;
            _length++;

            if ( _dataAvailSyn ) {
                _dataAvailSyn->signal();
            }

            rc = true;
        }

        return rc;
    }
}


/// @brief Empties the Pipe
void Pipe::flush()
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _startIndex = _length = 0;

        if ( _dataAvailSyn ) {
            _dataAvailSyn->clearSignals();
        }

        if ( _roomAvailSyn ) {
            _roomAvailSyn->signal();
        }
    }
}


/// @brief Assigns a read filter to the Pipe
/// @param f The callback function to use when data is about to read from the Pipe.
void Pipe::setReadFilter( PipeFilter f )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _readFilter = f;
    }
}


/// @brief Assigns a write filter to the Pipe
/// @param f The callback function to use when data is about to be written to the Pipe.
void Pipe::setWriteFilter( PipeFilter f )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _writeFilter = f;
    }
}


/// @brief Sets the Synapse to signal when room becomes available in the Pipe to store
/// more data
/// @param s The Synapse to signal.
void Pipe::setRoomAvailSynapse( Synapse& s )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _roomAvailSyn = &s;

        if ( !isFull() ) {
            _roomAvailSyn->signal();
        }
        else {
            _roomAvailSyn->clearSignals();
        }
    }
}


/// @brief Sets the Synapse to signal when data becomes available in the Pipe to read
/// @param s The Synapse to signal.
void Pipe::setDataAvailSynapse( Synapse& s )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _dataAvailSyn = &s;

        if ( !isEmpty() ) {
            _dataAvailSyn->signal();
        }
        else {
            _dataAvailSyn->clearSignals();
        }
    }
}


#endif
