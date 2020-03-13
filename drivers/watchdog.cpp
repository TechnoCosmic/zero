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

    WatchdogFlags _allocatedFlags{ 0 };
    WatchdogFlags _currentPats{ 0 };

}    // namespace


Watchdog::Watchdog()
:
    _flag{ allocateFlag() }
{
    // pat the dog straight away, for safety's sake
    if ( *this ) {
        pat();
    }
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


WatchdogFlags Watchdog::allocateFlag()
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
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
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _currentPats |= _flag;

        if ( _currentPats == _allocatedFlags ) {
            wdt_reset();
            _currentPats = 0;
        }
    }
}


#endif    // ZERO_DRIVERS_WDT