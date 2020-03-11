//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_WDT


#include <avr/wdt.h>
#include <util/atomic.h>
#include "watchdog.h"


using namespace zero;


namespace {

    WatchdogFlag _allocatedFlags{ 0 };
    WatchdogFlag _currentPats{ 0 };

}    // namespace


Watchdog::Watchdog()
:
    _flag{ allocateFlag() }
{
    // empty
}


Watchdog::~Watchdog()
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _currentPats &= ~_flag;
        _allocatedFlags &= ~_flag;
    }
}


Watchdog::operator bool() const
{
    return _flag;
}


WatchdogFlag Watchdog::allocateFlag()
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        for ( uint16_t i = 0; i < sizeof( WatchdogFlag ) * 8; i++ ) {
            const WatchdogFlag m{ 1U << i };

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
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _currentPats |= _flag;

        if ( _currentPats == _allocatedFlags ) {
            wdt_reset();
            _currentPats = 0;
        }
    }
}


#endif    // ZERO_DRIVERS_WDT