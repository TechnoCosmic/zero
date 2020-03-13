//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <avr/io.h>
#include <avr/power.h>
#include <util/atomic.h>

#include "gpio.h"
#include "power.h"
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
    // cache then clear the reset flags
    _resetFlags = ResetFlags( MCUSR );
    MCUSR = 0;

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


// Puts the MCU into super-coma
void Power::shutdown( const bool force, const bool silent )
{
    if ( _allowSleep or force ) {
        const auto mode = SLEEP_MODE_PWR_DOWN;

        cli();

        if ( !silent ) {
            onSleep( mode );
            cli();
        }

        Gpio::init();
        power_all_disable();

        set_sleep_mode( mode );
        sleep_enable();
        sleep_cpu();
    }
}


// Puts the MCU into idle mode
void Power::idle( const bool force, const bool silent )
{
    if ( _allowSleep or force ) {
        const auto mode = SLEEP_MODE_IDLE;

        cli();

        if ( !silent ) {
            onSleep( mode );
            cli();
        }

        set_sleep_mode( mode );
        sleep_enable();
        sei();
        sleep_cpu();
    }
}
