//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <avr/pgmspace.h>

#include "thread.h"
#include "cli/cli.h"
#include "examples/ledflasher.h"


using namespace zero;


// startup_sequence() is your new main().
// Start your Threads here, and other things that make sense to do so.
int startup_sequence()
{
    // empty CLI
    new Shell( 0, 9600 );

    // simple LED flasher
    new LedFlasher( PSTR( "led demo" ), ZERO_PINB5, 250, 750 );

    return 0;
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
