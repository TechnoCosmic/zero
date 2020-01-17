//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "core/thread.h"


using namespace zero;


// startup_sequence() is your new main().
// Initialize any GPIO or other things that make sense to do so.
// NOTE: You are not inside a Thread here. This function is called
// before zero has initialized itself fully. Consequently, you cannot
// call many zero functions from here. If you have enabled the DEBUG_
// options in the makefile, you can call debug::print() here.
void startup_sequence()
{
    ;
}


// This is the Thread that will run when no other Thread wants to. For
// this reason, you cannot block in the idle thread. You must be busy
// the whole time. This function has been exposed to the developer so
// that she can change the idle thread to perhaps flash an LED, or
// send some debugging text via debug::print() - something to indicate
// when the idle thread is running. Most of the time, you can use the
// supplied idle thread code and leave it at that.
int idleThreadEntry()
{
    while (true);
}
