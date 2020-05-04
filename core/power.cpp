//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


/// @file
/// @brief Contains classes and functions related to power, sleeping, and device reset


#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/atomic.h>

#include "power.h"
#include "gpio.h"
#include "watchdog.h"
#include "attrs.h"


using namespace zero;


namespace {

    auto _resetFlags{ ResetFlags::Unknown };
    auto _allowSleep{ true };

}


/// @brief Default reset handler
/// @details Implement your own version of this function to run code at start up, before
/// zero has initialized itself. You can see the reason(s) for the reset by inspecting the
/// supplied ResetFlags, and take action as appropriate.
/// @note Make sure you return `true` from this function. If you return `false`,
/// zero will NOT continue initialization and will put the MCU into a deep sleep,
/// requiring a hardware reset to wake up.
bool WEAK onReset( const ResetFlags )
{
    return true;
}


/// @brief Default sleep handler
/// @details This function is called before any attempt to sleep MCU using the Power
/// class. The exception to this is when `silent` is `true` in the call to
/// Power::sleep(). Implement your own version of this function to learn when the MCU is
/// about to have a nap.
void WEAK onSleep( const uint8_t )
{
    // empty
}


/// @brief Sets default power states, runs the reset handler
bool Power::init()
{
    // because the reset flags do not automatically
    // clear each reset (i.e. they're cumulative across
    // resets), we cache and clear the register here to
    // give us access to each unique reset event's info
    _resetFlags = ResetFlags( MCUSR );
    MCUSR = 0;

    // Watchdog init must be delayed until after MCUSR is
    // cleared (if the reset was caused by the WDT that is)
    #ifdef ZERO_DRIVERS_WDT
        Watchdog::init();
    #endif

    return onReset( _resetFlags );
}


/// @brief Gets the reset flags from the last power up
ResetFlags Power::getResetFlags()
{
    return _resetFlags;
}


/// @brief Allows the MCU to sleep when asked
/// @see isSleepEnabled(), preventSleep()
void Power::allowSleep()
{
    _allowSleep = true;
}


/// @brief Prevents the MCU from entering a sleep mode when asked
/// @see isSleepEnabled(), allowSleep()
void Power::preventSleep()
{
    _allowSleep = false;
}


/// @brief Determines if sleeping is currently allowed
/// @returns `true` if the MCU is allowed to sleep, `false` otherwise.
/// @see allowSleep(), preventSleep()
bool Power::isSleepEnabled()
{
    return _allowSleep;
}


/// @brief Puts the MCU to sleep
/// @param mode The desired sleep mode, from `avr/sleep.h`.
/// @param force Optional. Default: `false`. If `true`, the MCU will be forced into sleep even if currently prevented from doing so.
/// @param silent Optional. Default: `false`. If `true`, the onSleep() handler will not be called.
/// @see isSleepEnabled(), allowSleep(), preventSleep()
void Power::sleep( const uint8_t mode, const bool force, const bool silent )
{
    if ( _allowSleep or force ) {
        if ( !silent ) {
            onSleep( mode );
        }

        if ( mode == SLEEP_MODE_PWR_DOWN ) {
            Gpio::init();
            power_all_disable();
        }

        set_sleep_mode( mode );
        sleep_enable();

        if ( mode != SLEEP_MODE_PWR_DOWN ) {
            sei();
        }

        sleep_cpu();
    }
}
