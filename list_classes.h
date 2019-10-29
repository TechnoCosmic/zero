/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "namedobject.h"
#include "thread.h"

namespace zero {

    template class List<NamedObject>;
    template class List<Thread>;

}
