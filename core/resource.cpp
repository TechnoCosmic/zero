//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


/// @file
/// @brief Contains functions for controlling resource access


#include <util/atomic.h>
#include "resource.h"


using namespace zero;


namespace {

    uint16_t _resourceMap{ 0 };

}


/// @brief Obtains exclusive access to a resource
/// @param id The resource you wish to access.
/// @returns `true` if the resource was able to be reserved, `false` otherwise.
bool resource::obtain( const ResourceId id )
{
    const uint16_t m{ 1U << (uint16_t) id };

    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        if ( !( _resourceMap & m ) ) {
            _resourceMap |= m;
            return true;
        }

        return false;
    }
}


/// @brief Releases a previous held resource back to the system
/// @param id The resource to be released.
void resource::release( const ResourceId id )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _resourceMap &= ~( 1U << (uint16_t) id );
    }
}
