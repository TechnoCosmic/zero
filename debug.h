//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#ifndef TCRI_ZERO_DEBUG_H
#define TCRI_ZERO_DEBUG_H


namespace zero {

    namespace debug {

        // public
        void print(const char c);
        void print(const char* s, const bool fromFlash = false);

        // private
        void init();
    }

}


#endif