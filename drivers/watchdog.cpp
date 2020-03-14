//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_WDT


#include <avr/wdt.h>
#include "thread.h"
#include "watchdog.h"
#include "debug.h"


using namespace zero;


namespace {

    WatchdogFlags _allocatedFlags{ 0 };
    WatchdogFlags _currentPats{ 0 };

}    // namespace


void Watchdog::init()
{
    disable();
}


void Watchdog::enable( const uint8_t dur )
{
    wdt_enable( dur );
}


void Watchdog::disable()
{
    wdt_disable();
}


Watchdog::Watchdog()
:
    _flag{ []() -> WatchdogFlags
    {
        ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
            const auto shouldEnable{ !!!_allocatedFlags };
            const auto rc{ allocateFlag() };

            if ( shouldEnable and rc ) {
                enable( WATCHDOG_TIMEOUT );
            }

            return rc;
        }
    }() }
{
    if ( *this ) {
        pat();
    }
}


Watchdog::~Watchdog()
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        _currentPats &= ~_flag;
        _allocatedFlags &= ~_flag;

        if ( !_allocatedFlags ) {
            disable();
        }
    }
}


Watchdog::operator bool() const
{
    return _flag;
}


WatchdogFlags Watchdog::allocateFlag()
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        for ( uint16_t i = 0; i < sizeof( WatchdogFlags ) * 8; i++ ) {
            const WatchdogFlags m{ 1U << i };

            if ( !( _allocatedFlags & m ) ) {
                _allocatedFlags |= m;
                return m;
            }
        }

        return 0;
    }
}


void Watchdog::pat() const
{
    ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
        _currentPats |= _flag;

        if ( _currentPats == _allocatedFlags ) {
            wdt_reset();
            _currentPats = 0;
        }
    }
}


#endif    // ZERO_DRIVERS_WDT