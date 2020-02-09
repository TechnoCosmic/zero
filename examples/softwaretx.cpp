//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "softwaretx.h"
#include "thread.h"
#include "synapse.h"
#include "suart.h"


using namespace zero;


// ctor
SoftwareTx::SoftwareTx(
    const PinField txPins)
:
    // call parent ctor, with entryPoint as a lambda.
    // This is a stub that just calls ::main()
    Thread(0, []()
    {
        return ((SoftwareTx&) me).main();
    }),

    // other params
    _txPins{ txPins }
{
    // ctor body
}


// the main body of the Thread
int SoftwareTx::main()
{
    SuartTx tx;
    Gpio txPins(_txPins);
    Synapse txReadySyn;                                 // to learn when we can transmit again

    // make sure they all claimed their resources
    if (txPins && txReadySyn && tx) {
        // set up the communications
        tx.setCommsParams(9600, txPins);
        tx.enable(txReadySyn);

        // main loop
        while (true) {
            txReadySyn.wait();                          // wait for transmitter to be ready
            tx.transmit("Hello, World!\r\n", 15);       // send some text
        }
    }
    else {
        return 20;
    }
}
