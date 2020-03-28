//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_PIPE_H
#define TCRI_ZERO_PIPE_H


#ifdef ZERO_DRIVERS_PIPE


#include <stdint.h>
#include "thread.h"


namespace zero {

    typedef bool ( *PipeFilter )( uint8_t& data );

    class Pipe {
    public:
        Pipe( const uint16_t size );
        ~Pipe();

        explicit operator bool() const;

        bool isEmpty() const;
        bool isFull() const;

        bool read( uint8_t& data );
        bool write( const uint8_t data );
        void flush();

        void setReadFilter( PipeFilter p );
        void setWriteFilter( PipeFilter p );

        void setRoomAvailSynapse( Synapse& s );
        void setDataAvailSynapse( Synapse& s );

        #include "pipe_private.h"
    };

}    // namespace zero


zero::Pipe& operator<<( zero::Pipe& out, const char c );
zero::Pipe& operator<<( zero::Pipe& out, const char* s );


#endif

#endif
