//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "gpio.h"
#include "thread.h"


#ifdef ZERO_DRIVERS_GPIO
    template class List<Gpio>;
#endif

template class List<Thread>;
template class OffsetList<Thread>;
