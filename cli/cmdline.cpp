//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"
#include "cmdline.h"


using namespace zero;


namespace {
    const auto BACKSPACE = 8;
    const auto ESCAPE = 27;
}    // namespace


CommandLine::CommandLine( const uint16_t size )
:
    _buffer{ (char*) memory::allocate( size, &_bufferSize ) }
{
    if ( *this ) {
        clear();
    }
}


CommandLine::~CommandLine()
{
    memory::free( _buffer, _bufferSize );
}


// validity checking
CommandLine::operator bool() const
{
    return _buffer;
}


void CommandLine::clear()
{
    _cursor = 0;
    _buffer[ 0 ] = 0;
}


bool CommandLine::registerKeyPress( const char c )
{
    if ( c == ESCAPE ) {
        return false;
    }
    else if ( c == BACKSPACE ) {
        if ( _cursor ) {
            _buffer[ --_cursor ] = 0;
            return true;
        }
    }
    else {
        if ( _cursor < _bufferSize - 1 ) {
            _buffer[ _cursor++ ] = c;
            _buffer[ _cursor ] = 0;
            return true;
        }
    }

    return false;
}


void CommandLine::process()
{
    char* argv[ 16 ];
    const uint8_t argc = tokenize( _buffer, argv );

    if ( argc ) {
        // dispatch here
    }
}


uint8_t CommandLine::tokenize( char* s, char* argv[] )
{
    uint8_t tokenCount = 0;
    bool lastWasSeparator = true;
    bool inQuotes = false;

    while ( *s and tokenCount < 16 ) {
        if ( *s == '\"' ) {
            inQuotes = !inQuotes;
            *s = 0;
            lastWasSeparator = true;
        }
        else {
            if ( inQuotes or !isspace( *s ) ) {
                if ( lastWasSeparator ) {
                    argv[ tokenCount++ ] = s;
                }

                lastWasSeparator = false;
            }
            else if ( isspace( *s ) ) {
                *s = 0;
                lastWasSeparator = true;
            }
        }

        if ( tokenCount - 1 == 0 ) {
            *s = tolower( *s );
        }

        s++;
    }

    return tokenCount;
}
