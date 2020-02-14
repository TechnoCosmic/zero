//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_PIPE


#include "pipe.h"


zero::Pipe& operator<<( zero::Pipe& out, const char c )
{
    out.write( c );
    return out;
}


zero::Pipe& operator<<( zero::Pipe& out, const char* s )
{
    while ( *s ) {
        out.write( *s++ );
    }

    return out;
}


#endif
