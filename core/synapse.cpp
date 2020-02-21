//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "synapse.h"
#include "thread.h"


using namespace zero;


// ctor
Synapse::Synapse() :
    _thread{ &me },
    _signals{ me.allocateSignal() }
{
}


// dtor
Synapse::~Synapse()
{
    if ( _thread ) {
        _thread->freeSignals( _signals );
    }
}


// validity checking
Synapse::operator bool() const
{
    return _thread and _signals;
}


// For use as a simple SignalBitField
Synapse::operator SignalBitField() const
{
    return _signals;
}


// Signal the thread
void Synapse::signal() const
{
    if ( *this ) {
        _thread->signal( _signals );
    }
}


// Clears the signals
void Synapse::clearSignals() const
{
    if ( *this ) {
        _thread->clearSignals( _signals );
    }
}


// Waits for the signals to be set, blocking if necessary
SignalBitField Synapse::wait( const uint32_t timeoutMs ) const
{
    if ( *this and &me == _thread ) {
        return me.wait( _signals, timeoutMs );
    }
    else {
        return 0;
    }
}
