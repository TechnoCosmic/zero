//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "synapse.h"
#include "thread.h"


using namespace zero;


Synapse::Synapse() :
    _thread(&me),
    _signals(me.allocateSignal())
{
}


Synapse::~Synapse()
{
    _thread->freeSignals(_signals);
}


Synapse::operator bool() const
{
    return (_signals != 0UL);
}


Synapse::operator SignalField() const
{
    return _signals;
}


void Synapse::signal() const
{
    if (*this) {
        _thread->signal(_signals);
    }
}


void Synapse::clearSignals() const
{
    if (*this) {
        _thread->clearSignals(_signals);
    }
}


SignalField Synapse::wait() const
{
    if (*this && &me == _thread) {
        return me.wait(_signals);
    }
    else {
        return 0UL;
    }
}
