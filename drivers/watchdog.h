//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_WATCHDOG_H
#define TCRI_ZERO_WATCHDOG_H


#include <stdint.h>
#include <avr/wdt.h>


namespace zero {

    typedef uint16_t WatchdogFlags;

    /// @brief Provides multi-participant Watchdog timer services
    /// @details Sections of code can opt in and out of patting the WDT as required. When
    /// multiple threads or even multiple sections of code within a single thread are
    /// participating in WDT patting, all participants must call pat() on
    /// their Watchdog objects within the timeout, otherwise the AVR's WDT will not be
    /// reset, and a device reset will occur.
    /// @details If a piece of code no longer needs to be monitored by the WDT, then just
    /// let it's Watchdog object go out of scope. If there are no active Watchdog objects
    /// anywhere in the system, the AVR's WDT is automatically disabled. As soon as a new
    /// Watchdog object is created, the AVR's WDT is re-enabled.
    /// @details The timeout for the WDT is set in the ```makefile```, search
    /// for ```WATCHDOG_TIMEOUT```. It can be any acceptable version of ```WDTO_``` for
    /// your target MCU (see ```avr/wdt.h```).
    /// @details If you implement an onReset() handler in your project, you can
    /// check the supplied ResetFlags to see if the current power-up/reset event was
    /// triggered by a WDT timeout, and take action if necessary.
    /// @note There is a maximum of 16 active Watchdog objects/participants at any one
    /// time. Be sure to check that your dog initialized correctly before proceeding.
    /// @note You can use a Watchdog in your onReset() handler if you have one, and also
    /// in main().
    /// @details Another use for the Watchdog is to make the idle thread a participant.
    /// You might want to do this to check that your program isn't endlessly busy when it
    /// shouldn't be. You can override the default idle thread code by supplying your own
    /// idleThreadEntry(). For example...
    /// @code
    /// int idleThreadEntry()
    /// {
    ///     Watchdog dog;
    ///
    ///     while ( true ) {
    ///         Power::sleep( SLEEP_MODE_IDLE );
    ///         
    ///         if ( dog ) {
    ///             dog.pat();
    ///         }
    ///     }
    /// }
    /// @endcode
    /// @details This gives you an idle thread that puts the MCU into idle sleep mode, and
    /// when it wakes up, it will pat the Watchdog. What this means is that if your
    /// program is so busy that the idle thread doesn't get to run at least for one moment
    /// between WDT timeouts, then the MCU will reset.

    class Watchdog {
    public:
        static void enable( const uint8_t dur );
        static void disable();

        Watchdog();

        explicit operator bool() const;

        void pat() const;

        #include "watchdog_private.h"
    };

}    // namespace zero


#endif    // TCRI_ZERO_WATCHDOG_H
