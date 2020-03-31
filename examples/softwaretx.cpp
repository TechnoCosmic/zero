//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "softwaretx.h"
#include "thread.h"
#include "suart.h"


using namespace zero;


// ctor
SoftwareTx::SoftwareTx(
    const char* const name,
    const PinField txPins )
:
    // call parent ctor, with entryPoint as a lambda.
    // This is a stub that just calls ::main()
    Thread( name, 0, []()
    {
        return ( (SoftwareTx&) me ).main();
    } ),

    // other init
    _txPins{ txPins }
{
    // empty
}


// the main body of the Thread
int SoftwareTx::main()
{
    Gpio txPins{ _txPins };
    Synapse txReadySyn;
    SuartTx tx{ 9600, txPins, txReadySyn };

    // make sure they all claimed their resources
    if ( txPins and txReadySyn and tx ) {
        // main loop
        while ( true ) {
            txReadySyn.wait();                          // wait for transmitter to be ready
            tx.transmit( "Hello, World!\r\n", 15 );     // send some text
        }
    }
    else {
        return 20;
    }
}
