//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//

#include <avr/pgmspace.h>
#include "thread.h"


using namespace zero;


int myThreadMain()
{
    // put your thread code here

    while ( true ) {
        // empty
    }

    // and if you exit, be sure to return something
    return 0;
}


// Set things up, spawn your initial Threads. Get your party started here.
// But let main() exit - only once main() returns will the kernel finish initializing.
int main()
{
    new Thread( PSTR( "myThreadName" ), 256, myThreadMain );
}
