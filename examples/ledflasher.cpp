//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "ledflasher.h"
#include "thread.h"
#include "gpio.h"


using namespace zero;


LedFlasher::LedFlasher(
    const char* const name,
    const PinField ledPins,
    const Duration delay,
    const int count )
:
    LedFlasher{ name, ledPins, delay, delay, count }
{
    // empty
}


LedFlasher::LedFlasher(
    const char* const name,
    const PinField ledPins,
    const Duration timeOn,
    const Duration timeOff,
    const int count)
:
    // call parent ctor, with entryPoint as a lambda.
    // This is a stub that just calls ::main()
    Thread{ name, 0, []()
    {
        return ( ( LedFlasher& ) me ).main();
    } },

    // other init
    _ledPins{ ledPins },
    _timeOn{ timeOn },
    _timeOff{ timeOff },
    _flashesRemaining{ count }
{
    // empty
}


// the main body of the Thread
int LedFlasher::main()
{
    Gpio led{ _ledPins };

    if ( !led ) {
        return 20;
    }

    led.setAsOutput();

    while ( true ) {
        led.switchOn();
        me.delay( _timeOn );

        led.switchOff();
        me.delay( _timeOff );

        if ( _flashesRemaining > 0 ) {
            _flashesRemaining--;

            if ( !_flashesRemaining ) {
                break;
            }
        }
    }

    return 0;
}
