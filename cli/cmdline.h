//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_CMDLINE_H
#define TCRI_ZERO_CMDLINE_H


#include <stdint.h>


namespace zero {

    class CommandLine {
    public:
        CommandLine( const uint16_t size );
        ~CommandLine();

        bool registerKeyPress( const char c );
        void clear();
        void process();

        explicit operator bool() const;

    private:
        uint8_t tokenize( char* s, char* argv[] );

        char* _buffer;
        uint16_t _bufferSize;
        uint16_t _cursor;
    };

}    // namespace zero


#endif
