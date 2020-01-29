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
    _thread(&me),
    _signals(me.allocateSignal())
{
}


// dtor
Synapse::~Synapse()
{
    if (_thread) {
        _thread->freeSignals(_signals);
    }
}


// For isValid determination
Synapse::operator bool() const
{
    return (_signals != 0UL);
}


// For use as a simple SignalField
Synapse::operator SignalField() const
{
    return _signals;
}


// Signal the thread
void Synapse::signal() const
{
    if (*this) {
        _thread->signal(_signals);
    }
}


// Clears the signals
void Synapse::clearSignals() const
{
    if (*this) {
        _thread->clearSignals(_signals);
    }
}


// Waits for the signal(s)
SignalField Synapse::wait() const
{
    if (*this && &me == _thread) {
        return me.wait(_signals);
    }
    else {
        return 0UL;
    }
}
