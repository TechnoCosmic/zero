//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <avr/wdt.h>
#include "thread.h"
#include "watchdog.h"
#include "debug.h"


using namespace zero;


namespace {

    #ifdef ZERO_DRIVERS_WDT
        WatchdogFlags _allocatedFlags{ 0 };
        WatchdogFlags _currentPats{ 0 };
    #endif

}    // namespace


void Watchdog::init()
{
    #ifdef ZERO_DRIVERS_WDT
        disable();
    #endif
}


#ifdef ZERO_DRIVERS_WDT
    void Watchdog::enable( const uint8_t dur )
    {
        wdt_enable( dur );
    }
#else
    void Watchdog::enable( const uint8_t )
    {
        // empty
    }
#endif


void Watchdog::disable()
{
    #ifdef ZERO_DRIVERS_WDT
        wdt_disable();
    #endif
}


#ifdef ZERO_DRIVERS_WDT
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
#else
    Watchdog::Watchdog()
    :
        _flag{ 0 }
    {
        // empty
    }
#endif


Watchdog::~Watchdog()
{
    #ifdef ZERO_DRIVERS_WDT
        ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
            _currentPats &= ~_flag;
            _allocatedFlags &= ~_flag;

            if ( !_allocatedFlags ) {
                disable();
            }
        }
    #endif
}


Watchdog::operator bool() const
{
    #ifdef ZERO_DRIVERS_WDT
        return _flag;
    #else
        return true;
    #endif
}


WatchdogFlags Watchdog::allocateFlag()
{
    #ifdef ZERO_DRIVERS_WDT
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
    #endif
}


void Watchdog::pat() const
{
    #ifdef ZERO_DRIVERS_WDT
        ZERO_ATOMIC_BLOCK ( ZERO_ATOMIC_RESTORESTATE ) {
            _currentPats |= _flag;

            if ( _currentPats == _allocatedFlags ) {
                wdt_reset();
                _currentPats = 0;
            }
        }
    #endif
}
