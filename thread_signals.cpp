/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"
#include "thread.h"

using namespace zero;


const uint8_t SIGNAL_BITS = sizeof(SignalMask) * 8;


// tries to allocate the given signal number, returns true if it was available
bool Thread::tryAllocSignal(const int8_t sigNum) {
    const SignalMask m = 1L << sigNum;

    // if it isn't yet allocated
    if (!(_allocatedSignals & m)) {
        // allocate it
        _allocatedSignals |= m;

        // and return the signal NUMBER (not the mask)
        return true;
    }

    return false;
}


// reserves a signal number for use
int8_t Thread::allocateSignal(const int8_t reqdSignal) {
    if (reqdSignal == -1) {
        // search each possible bit
        for (int8_t i = 0; i < SIGNAL_BITS; i++) {
            if (tryAllocSignal(i)) {
                return i;
            }
        }

    } else {
        if (tryAllocSignal(reqdSignal)) {
            return reqdSignal;
        }
    }

    // this is the "no available signals" return code
    return -1;
}


// frees up (deallocates) a previously allocated signal
void Thread::freeSignal(const uint8_t signalNumber) {
    // no point in checking if it's allocated or not, just free it
    _allocatedSignals &= ~(1L << signalNumber);

    // make sure the deallocated signal is removed from other places
    _waitingSignals &= _allocatedSignals;
    _currentSignals &= _allocatedSignals;
}


// A Thread calls this to block itself until certain
// signals are received
SignalMask Thread::wait(const SignalMask signals) {
    SignalMask rc = 0;

    // Threads can only wait on their own signals
    if (Thread::me() != this) return 0;

    // santise the signals
    const SignalMask sanitsedSignals = signals & _allocatedSignals;

    // add these signals into the ones we're waiting on
    _waitingSignals = sanitsedSignals;

    // only block if we're actually waiting on something
    // that we haven't received yet
    if (!getActiveSignals()) {
        block(TS_PAUSED);
    }

    // return the portion of the waiting signals
    // that we actually received.
    
    // NOTE: Do *NOT* optimize this away by caching
    // the result of getActiveSignals() above. If we
    // blocked, then that cached value would (and
    // should) be out-of-date by this point
    rc = getActiveSignals();

    // remove those signals from the
    // currently unhandled signals
    _currentSignals &= ~rc;

    // return the signals that woke us up
    return rc;
}


// Signals the Thread - all this does is set the signals - it does NOT change states
void Thread::signal(const SignalMask newSignals) {
    const SignalMask sanitizedSignals = newSignals & _allocatedSignals;
    _currentSignals |= sanitizedSignals;
}


// returns the current signals affecting the Thread
SignalMask Thread::getCurrentSignals() {
    return _currentSignals;
}


// returns the signals the Thread is waiting on
SignalMask Thread::getWaitingSignals() {
    return _waitingSignals;
}


// see which signals that we care about are currently active
SignalMask Thread::getActiveSignals() {
    return _waitingSignals & _currentSignals & _allocatedSignals;
}