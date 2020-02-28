//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <avr/io.h>
#include "power.h"
#include "attrs.h"


using namespace zero;


namespace {

    static auto _resetFlags{ ResetFlags::Unknown };

}


// default reset handler
bool WEAK onReset( const ResetFlags )
{
    return true;
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
