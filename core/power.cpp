//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <avr/io.h>
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


// default reset handler
bool WEAK onReset( const ResetFlags )
{
    return true;
}


// default sleep handler
void WEAK onSleep( const uint8_t )
{
    // empty
}


// set default power states, run reset handler
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


// Returns the reset flags from the last power up
ResetFlags Power::getResetFlags()
{
    return _resetFlags;
}


void Power::allowSleep()
{
    _allowSleep = true;
}


void Power::preventSleep()
{
    _allowSleep = false;
}


bool Power::isSleepEnabled()
{
    return _allowSleep;
}


// Puts the MCU into unwakeable super-coma
void Power::sleep( const uint8_t mode, const bool force, const bool silent )
{
    if ( _allowSleep or force ) {
        cli();

        if ( !silent ) {
            onSleep( mode );
            cli();
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
