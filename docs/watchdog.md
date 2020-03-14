# Watchdog
```zero/drivers/watchdog.h```

The ```Watchdog``` class provides a dynamic automated WDT management system.

Sections of code can opt in and out of patting the WDT as required. When multiple threads or even multiple sections of code within a single thread are participating in WDT patting, all participants must call ```::pat()``` on their ```Watchdog``` objects within the timeout, otherwise the AVR's WDT will not be reset, and a device reset will occur.

If a piece of code no longer needs to be monitored by the WDT, then just let it's ```Watchdog``` object go out of scope. If there are no active ```Watchdog``` objects anywhere in the system, the AVR's WDT is automatically disabled. As soon as any ```Watchdog``` object is created, the AVR's WDT is [re]enabled.

## Example
```
#include "thread.h"
#include "debug.h"
#include "watchdog.h"

int myThread()
{
    while ( true ) {
        // do some work here...

        // busy busy

        // now here's something that we want protected by the WDT
        if ( doingCriticalJob ) {
            Watchdog dog;

            if ( dog ) {
                while ( !exitCondition ) {
                    // whatever happens in here, make sure you pat
                    // your Watchdog before the timeout expires
                    dog.pat();

                    // busy busy

                    me.delay( 250_ms );
                }
            }
            else {
                dbg_assert( false, "No available Watchdogs" );
            }
        }

        // because 'dog' fell out-of-scope, any code running
        // here is not required to pat the WDT and is
        // therefore not protected by it either

        // busy busy
    }
}
```
The timeout for the WDT is set in the ```makefile```, search for ```WATCHDOG_TIMEOUT```. It can be any acceptable version of ```WDTO_``` for your target MCU (see ```avr/wdt.h```).

If you implement an ```onReset()``` handler in your project, you can check the supplied ```ResetFlags``` to see if the current power-up/reset event was triggered by a WDT timeout, and take action if necessary.

**NOTE:** There is a maximum of 32 active ```Watchdog``` objects/participants at any one time. Be sure to check that your dog initialized correctly before proceeding.

**NOTE:** You can use a ```Watchdog``` in your ```onReset()``` handler if you have one, and also in ```main()```.
