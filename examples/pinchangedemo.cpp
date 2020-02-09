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
    const PinField pins)
:
    // call parent ctor, with entryPoint as a lambda.
    // This is a stub that just calls ::main()
    Thread(0, []()
    {
        return ((PinChangeDemo&) me).main();
    }),

    // other params
    _pins{pins}
{
    // ctor body
}


int PinChangeDemo::main()
{
    Synapse listenSyn;
    Gpio listenPins(_pins, &listenSyn);

    if (!listenPins) {
        return 20;
    }

    listenPins.setAsInput();
    listenPins.switchOn();

    while (true) {
        listenSyn.wait();
        dbg_pgm("Input changed!\r\n");
    }
}
