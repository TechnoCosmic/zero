//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_POWER_H
#define TCRI_ZERO_POWER_H


#include <stdint.h>


namespace zero {


    enum ResetFlags {
        Unknown = 0,
        PowerOn = ( 1 << 0 ),
        External = ( 1 << 1 ),
        Brownout = ( 1 << 2 ),
        Watchdog = ( 1 << 3 ),
        Jtag = ( 1 << 4 ),
    };


    class Power {
    public:
        static bool init();
        static ResetFlags getResetFlags();        
    };

}


#endif