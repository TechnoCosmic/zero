//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "thread.h"


using namespace zero;


/// @brief Creates a new Synapse
Synapse::Synapse()
:
    _thread{ &me },
    _signals{ me.allocateSignal() }
{
    // empty
}


// dtor
Synapse::~Synapse()
{
    if ( _thread ) {
        _thread->freeSignals( _signals );
    }
}


/// @brief Determines if the Synapse initialized correctly
Synapse::operator bool() const
{
    return _thread and _signals;
}


/// @brief Extracts the signals from the Synapse
/// @returns A SignalBitField containing all the signals represented by the Synapse.
Synapse::operator SignalBitField() const
{
    return _signals;
}


/// @brief Signals the Thread represented by the Synapse
void Synapse::signal() const
{
    if ( *this ) {
        _thread->signal( _signals );
    }
}


/// @brief Clears the signals represented by the Synapse
void Synapse::clearSignals() const
{
    if ( *this ) {
        _thread->clearSignals( _signals );
    }
}


/// @brief Waits for the signals to be set, blocking if necessary
/// @param timeout Optional. Default: `0_ms` (no timeout). The maximum length of time
/// to wait to receive the signals.
SignalBitField Synapse::wait( const Duration timeout ) const
{
    if ( *this and &me == _thread ) {
        return me.wait( _signals, timeout );
    }
    else {
        return 0;
    }
}
