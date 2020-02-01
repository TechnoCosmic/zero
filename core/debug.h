//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_DEBUG_H
#define TCRI_ZERO_DEBUG_H


namespace zero {

    class debug {
    public:

        // public
        static void print(const char c);
        static void print(const char* s, const bool fromFlash = false);
        static void print(const uint16_t n, const int base = 10);

        #include "debug_private.h"
    };

}

#ifdef DEBUG_ENABLED
    #define dbg(x) zero::debug::print(x)
    #define dbg_pgm(x) zero::debug::print((char*)(PSTR(x)), true)
    #define dbg_int(x) zero::debug::print((x), 10)
#else
    #define dbg(x) ;
    #define dbg_pgm(x) ;
    #define dbg_int(x) ;
#endif


#endif