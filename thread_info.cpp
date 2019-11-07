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


// returns the address of the bottom of the Thread's stack
uint16_t Thread::getStackBottom() {
	return (uint16_t) _stackBottom;
}


// returns the address of the top of the Thread's stack
uint16_t Thread::getStackTop() {
	return ((uint16_t) _stackBottom) + _stackSize - 1;
}


// returns the size of the Thread's stack, in bytes
uint16_t Thread::getStackSizeBytes() {
	return _stackSize;
}


// returns true if the Thread was created dyanmically, false if it was globally declared
bool Thread::isDynamic() {
	return ((uint16_t) this) == getStackBottom();
}


// returns the number of bytes of stack used at the last context switch
uint16_t Thread::calcCurrentStackBytesUsed() {
	int extra = isDynamic() ? sizeof(Thread) : 0;
	return (getStackTop() - _sp) + extra;
}


// Who am I?
Thread* Thread::me() {
	return _currentThread;
}


#ifdef INSTRUMENTATION


// returns the most number of bytes of stack used at any context switch
uint16_t Thread::calcPeakStackBytesUsed() {
	int extra = isDynamic() ? sizeof(Thread) : 0;
	return (getStackTop() - _lowestSp) + extra;
}


#endif
