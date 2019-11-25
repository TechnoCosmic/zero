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


// returns the unique ID of the Thread
uint16_t Thread::getThreadId() {
	return _tid;
}


// sets the new state of the Thread
void Thread::setState(const ThreadState newState) {
	_state = newState;
}


// gets the current state of the Thread
ThreadState Thread::getState() {
	ThreadState rc = _state;

	// we had a timeout set, and it has expired, we are ready to run
	if (rc == TS_PAUSED && _wakeUpTime > 0 && Thread::now() >= _wakeUpTime) {
		_wakeUpTime = 0;
		signal(SIGMSK_TIMEOUT);
	}

	// if anything is signalled to us that we care about, we are ready to run!
	if (rc == TS_PAUSED && getActiveSignals()) {
		_state = rc = TS_READY;
	}

	return rc;
}

// returns the address of the bottom of the Thread's stack
uint16_t Thread::getStackBottom() {
	return (uint16_t) _stackBottom;
}


// returns the address of the top of the Thread's stack
uint16_t Thread::getStackTop() {
	return ((uint16_t) _stackBottom) + _stackSizeBytes - 1;
}


// returns the size of the Thread's stack, in bytes
uint16_t Thread::getStackSizeBytes() {
	return _stackSizeBytes;
}


// returns the number of bytes of stack used at the last context switch
uint16_t Thread::calcCurrentStackBytesUsed() {
	return getStackTop() - _sp;
}


#ifdef INSTRUMENTATION


// returns the most number of bytes of stack used at any context switch
uint16_t Thread::calcPeakStackBytesUsed() {
	return getStackTop() - _lowestSp;
}


#endif
