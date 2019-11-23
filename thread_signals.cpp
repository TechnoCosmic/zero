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


// reserves a signal number for use
uint8_t Thread::allocateSignal() {
    for (uint8_t i = 0; i < SIGNAL_BITS; i++) {
        const SignalMask m = 1L << i;

        if (!(_allocatedSignals & m)) {
            _allocatedSignals |= m;
            return i;
        }
    }
    return -1;
}


// frees up (deallocates) a previously allocated signal
void Thread::freeSignal(const uint8_t signalNumber) {
    _allocatedSignals &= ~(1L << signalNumber);
}


SignalMask Thread::wait(const SignalMask signals) {
    // clear the current signals
    _currentSignals = 0UL;

    // make sure we only wait on allocated signals
    _waitingSignals = signals & _allocatedSignals;

    // only block if we're actually waiting on something
    if (_waitingSignals) {
        block(TS_PAUSED, 0UL);
    }

    // return the signals that woke us up
    return _currentSignals;
}


// Signals the Thread
void Thread::signal(const SignalMask receivedSignals) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
        if (_state == TS_PAUSED) {
            // clean signals = whatever was sent us that were also allocated
            const SignalMask cleanSignals = receivedSignals & _allocatedSignals;

            // if we were waiting on any of those signals
            if (cleanSignals & _waitingSignals) {
                // set the signals
                _currentSignals |= cleanSignals;

                // wake up
                _state = TS_READY;
            }
        }
    }
}


// returns the current signals affecting the Thread
SignalMask Thread::getCurrentSignals() {
    return _currentSignals;
}


// returns the signals the Thread is waiting on
SignalMask Thread::getWaitingSignals() {
    return _waitingSignals;
}
