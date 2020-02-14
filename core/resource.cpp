//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <util/atomic.h>
#include "resource.h"


using namespace zero;


namespace {
    uint16_t _resourceMap = 0UL;
}


bool resource::obtain( const ResourceId id )
{
    const uint16_t m = 1L << (uint16_t) id;

    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        if ( !( _resourceMap & m ) ) {
            _resourceMap |= m;
            return true;
        }

        return false;
    }
}


void resource::release( const ResourceId id )
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        _resourceMap &= ~( 1L << (uint16_t) id );
    }
}
