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


/// @brief Creates a new DoubleBuffer of a given size
/// @param size The size of the buffer, in bytes.
DoubleBuffer::DoubleBuffer( const uint16_t size )
:
    _buffer{ (uint8_t*) memory::allocate( size, &_bufferSize ) },
    _pivot{ _bufferSize / 2 },
    _writeOffset{ 0 },
    _usedBytes{ 0 }
{
    // empty
}


// dtor
DoubleBuffer::~DoubleBuffer()
{
    memory::free( _buffer, _bufferSize );
}


/// @brief Determines if the DoubleBuffer initialized correctly
/// @returns `true` if the DoubleBuffer initialized correctly, `false` otherwise.
DoubleBuffer::operator bool() const
{
    return _buffer;
}


/// @brief Writes a byte to the buffer
/// @param d The byte to write to the buffer.
/// @returns `true` if the byte was successfully written, `false` if the buffer is
/// full.
bool DoubleBuffer::write( const uint8_t d )
{
    bool rc{ false };
    const uint8_t oldSreg{ SREG };
    cli();

    if ( _usedBytes < _pivot ) {
        _buffer[ _writeOffset + _usedBytes ] = d;
        _usedBytes++;

        rc = true;
    }

    SREG = oldSreg;

    return rc;
}


/// @brief Returns the currently active half of the buffer
/// @param numBytes A place to store the number of valid bytes in the buffer.
/// @returns A pointer to the active half of the buffer, or `nullptr` if the buffer is
/// empty.
uint8_t* DoubleBuffer::getCurrentBuffer( uint16_t& numBytes )
{
    const uint8_t oldSreg{ SREG };
    cli();

    uint8_t* rc{ nullptr };

    // tell the caller how many bytes we have
    numBytes = _usedBytes;

    // clear ready for more data
    _usedBytes = 0;

    // return the current half of the buffer
    if ( numBytes ) {
        rc = &_buffer[ _writeOffset ];
    }

    // swap to the other half
    _writeOffset = _writeOffset == 0 ? _pivot : 0;

    // restore SREG and escape
    SREG = oldSreg;

    return rc;
}


/// @brief Clears the buffer
void DoubleBuffer::flush()
{
    const uint8_t oldSreg{ SREG };
    cli();

    _writeOffset = 0;
    _usedBytes = 0;

    SREG = oldSreg;
}
