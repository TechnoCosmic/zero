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
        static void print(const char c);
        static void print(const char* s, const bool fromFlash = false);

        // private
        static void init();
    };

}


#define dbg(x) debug::print(x, false);
#define dbg_pgm(x) debug::print(PSTR(x), true)


#endif