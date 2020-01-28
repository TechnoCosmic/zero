//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include "synapse.h"


using namespace zero;


Synapse::Synapse()
{
    clear();
}


Synapse::Synapse(const SignalField sigs)
{
    thread = &Thread::getCurrentThread();
    signals = sigs;
}


void Synapse::clear()
{
    thread = nullptr;
    signals = 0UL;
}


bool Synapse::isValid() const
{
    return (thread != nullptr && signals != 0UL);
}


void Synapse::signal() const
{
    if (isValid()) {
        thread->signal(signals);
    }
}


void Synapse::clearSignals() const {
    if (isValid()) {
        thread->clearSignals(signals);
    }
}


void Synapse::wait() const {
    if (isValid()) {
        if (thread == &Thread::getCurrentThread()) {
            thread->wait(signals);
        }
    }
}
