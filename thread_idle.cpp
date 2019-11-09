/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <avr/io.h>
#include "zero_config.h"
#include "thread.h"
#include "memory.h"
#include "util.h"

using namespace zero;


static const PROGMEM char _idleThreadName[] = "idle";

static int idle() {
	while(1);
}

// creates the idle thread, for running when nothing else wants to
Thread* Thread::createIdleThread() {
	uint16_t allocated = 0UL;
	uint16_t requestedStackBytes = MAX(IDLE_THREAD_STACK_BYTES, THREAD_MIN_STACK_BYTES);
	uint8_t* stackBottom = memory::allocate(requestedStackBytes, &allocated, THREAD_MEMORY_SEARCH_STRATEGY);
	Thread* newThread = (Thread*) stackBottom;

	if (newThread) {
		newThread->configureThread(_idleThreadName, stackBottom, allocated, 0, idle, TLF_READY | TLF_AUTO_CLEANUP);
	}
	
	return newThread;
}
