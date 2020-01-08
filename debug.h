//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#ifndef TCRI_ZERO_DEBUG_H
#define TCRI_ZERO_DEBUG_H


namespace zero {

    class debug {
    public:

        // public
        static void print(char c);
        static void print(char* s, const bool fromFlash = false);

        // private
        static void init();
    };

}


#define dbg(x) zero::debug::print(x)
#define dbg_pgm(x) zero::debug::print(PSTR(x), true)


#endif