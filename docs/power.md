# Power
```zero/core/power.h```

The ```Power``` class provides power management services.


## onReset()
```onReset()``` is an optional handler that runs before ```main()```. If you supply your own ```onReset()``` handler, make sure you return ```true``` if the MCU should continue to initialize as normal. If you return ```false```, zero will put the MCU to sleep without any further initialization.

The only parameter to ```onReset()``` is a ```ResetFlags``` which is a bit field specifying the condition(s) which caused the most recent reset.

**NOTE:** If you return ```false``` from ```onReset()```, the only way to restart the MCU is by power cycling, or by external reset.

**NOTE:** Do not call ```onReset()``` - this is a function you supply if you want to take any action before ```main()``` runs, such as checking battery state or other hardware conditions.

**NOTE:** The code in ```onReset()``` is not running in the context of a ```Thread```. It is run prior to ```main()```, and the only sub-systems initialized at this time are debug print/assert, and the ```Gpio``` subsystem. No timers are active, the kernel hasn't be initialized - nothing else is running.

### Example
```
#include "power.h"

bool onReset( const ResetFlags rf )
{
    if ( rf & ResetFlags::PowerOn ) {
        dbg_pgm( "Power on\r\n" );
    }

    if ( rf & ResetFlags::External ) {
        dbg_pgm( "External reset\r\n" );
    }

    if ( rf & ResetFlags::Brownout ) {
        dbg_pgm( "Brownout reset\r\n" );

        // we won't let the MCU fire up if
        // the battery is borderline
        return false;
    }

    if ( rf & ResetFlags::Watchdog ) {
        dbg_pgm( "Watchdog reset\r\n" );
    }

    if ( rf & ResetFlags::Jtag ) {
        dbg_pgm( "JTAG reset\r\n" );
    }

    return true;
}
```
