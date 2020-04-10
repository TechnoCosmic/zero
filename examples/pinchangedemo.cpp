//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "pinchangedemo.h"
#include "debug.h"


using namespace zero;


// ctor
PinChangeDemo::PinChangeDemo(
    const char* const name,
    const PinField pins)
:
    // call parent ctor, with entryPoint as a lambda.
    // This is a stub that just calls ::main()
    Thread{ name, 0, []()
    {
        return ( ( PinChangeDemo& ) me ).main();
    } },

    // other init
    _pins{ pins }
{
    // empty
}


int PinChangeDemo::main()
{
    Synapse listenSyn;
    Gpio listenPins{ _pins, listenSyn };

    if ( !listenPins ) {
        return 20;
    }

    // set up input, with pull-ups enabled
    listenPins.setAsInput( PinControl::Locked );
    listenPins.switchOn( PinControl::Locked );

    while ( true ) {
        listenSyn.wait();

        if ( listenPins.getInputState() ) {
            dbg_pgm( "Button up!\r\n" );
        }
        else {
            dbg_pgm( "Button down!\r\n" );
        }
    }
}
