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

    /// @brief Callback function for Pipe filters
    /// @param data The byte being read from or written to the Pipe.
    typedef bool ( *PipeFilter )( uint8_t& data );

    /// @brief Thread-safe FIFO buffer for IPC
    class Pipe {
    public:
        Pipe( const uint16_t size );

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
