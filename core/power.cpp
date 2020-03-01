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


// Puts the MCU into super-coma
void Power::shutdown()
{
    cli();
    onSleep( SLEEP_MODE_PWR_DOWN );
    cli();

    Gpio::init();
    power_all_disable();

    set_sleep_mode( SLEEP_MODE_PWR_DOWN );
    sleep_enable();
    sleep_cpu();
}


// Puts the MCU into idle mode
void Power::idle()
{
    cli();
    onSleep( SLEEP_MODE_IDLE );
    cli();

    set_sleep_mode( SLEEP_MODE_IDLE );
    sleep_enable();
    sei();
    sleep_cpu();
}
