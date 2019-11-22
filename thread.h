/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_THREAD_H
#define TCRI_ZERO_THREAD_H


#include "zero_config.h"

#include <stdint.h>
#include <avr/pgmspace.h>
#include "namedobject.h"


#define ZERO_BUILD_VERSION 0
#define ZERO_BUILD_REVISION 9

static const uint8_t THREAD_MIN_STACK_BYTES = 48;

namespace zero {

	typedef int (*ThreadEntryPoint)();

	enum ThreadState {
		TS_RUNNING,								// Thread is currently executing. Only one Thread can have this state at a time
		TS_READY,								// Thread is ready to run at the next opportunity to do so
		TS_PAUSED,								// Thread is suspended and must be programmatically made ready via ::run()
		TS_WAITING,								// Thread is blocked until a specific time
		TS_JOINING,								// Thread is blocked waiting for another Thread to terminate
		TS_WAIT_ATOMIC_WRITE,					// Thread is blocked, waiting for exclusive write access to a Pipe
		TS_PIPE_READ,							// Thread is blocked, waiting for a Pipe to contain data to read
		TS_PIPE_WRITE,							// Thread is blocked, waiting to room to be created in a Pipe so it can write to it
		TS_TERMINATED,							// Thread is terminated, will not run again, and is awaiting cleanup
	};

	// Thread Launch Flags - these control various aspects
	// of how a Thread starts up and shuts down

	const uint16_t TF_READY = 1;				// Thread is ready to run as soon as the scheduler allows
	const uint16_t TF_AUTO_CLEANUP = 2;			// Thread will clean up after itself upon termination
	const uint16_t TF_QUICK = 4;				// Quick launch - bypass niceties like clearing stack memory
	const uint16_t TF_POOL = 8;					// Thread is part of a Thread Pool. Tells the cleanup to recycle.

	// Thread class
	class Thread {
	public:
		// kickstarts the scheduler
		static void init();

		// Thread creation - two ctors for convenience
		Thread(const uint16_t stackSizeBytes, const ThreadEntryPoint entry, const int flags = TF_READY | TF_AUTO_CLEANUP);
		Thread(const char* name, const uint16_t stackSizeBytes, const uint8_t quantumOverride, const ThreadEntryPoint entryPoint, const int flags = TF_READY | TF_AUTO_CLEANUP);

		// general blocking and unblocking
		static void block(const ThreadState newState, const uint32_t blockInfo);
		static void unblock(const ThreadState state, const uint32_t blockInfo);

		// Thread management (static)
		static Thread* me();					// for applications to find themselves
		static void forbid();					// prevents pre-emptive context switching
		static void permit();					// allows pre-emptive context switching
		static bool isSwitchingEnabled();		// determines if context switching is on

		// Thread management (member)
		bool setParameter(const uint8_t parameterNumber, const uint16_t value);
		bool run();								// allows a paused Thread to run
		bool pause();							// pauses a running/ready Thread
		int join();								// waits for a Thread to terminate
		bool remove();							// removes a Thread from the readylist
		bool cleanup();							// cleans up memory after a Thread is terminated
		void waitUntil(const uint32_t untilMs);	// blocks a Thread until a given time

		// Properties (thread_info.cpp)
		uint16_t getThreadId();					// returns the Thread's unique ID
		uint16_t getStackBottom();				// returns the bottom of the Thread's allocated stack
		uint16_t getStackTop();					// returns the top of the Thread's allocated stack
		uint16_t getStackSizeBytes();			// returns the size of the Thread's stack, in bytes
		uint16_t calcCurrentStackBytesUsed();	// returns the current stack usage, in bytes

#ifdef INSTRUMENTATION
		uint16_t calcPeakStackBytesUsed();		// returns the peak stack usage, in bytes
#endif
		// end (thread_info.cpp)

		// miscellaneous, but sort of related!
		static uint64_t now();					// returns the number of milliseconds since the scheduler was initialized

		// ********* DATA *********

		NamedObject _systemData;				// NamedObject must be first
		Thread* _prev;							// Thread lists previous
		Thread* _next;							// Thread lists next

		uint8_t _sreg;							// status register
		uint16_t _sp;							// stack pointer

	#ifdef RAMPZ
		uint8_t _rampz;							// RAMPZ
	#endif

		// main control block
		uint16_t _tid;							// Thread ID
		ThreadState _state;						// current state
		uint32_t _blockInfo;					// info related to current state (if blocked)
		int (*_entryPoint)();					// main entry for the Thread
		uint8_t _quantumTicks;					// the size of this Thread's quantum, in ticks
		uint8_t _remainingTicks;				// time left in the current quantum, in ticks
		uint16_t _flags;						// special flags
		int _exitCode;							// return code upon termination

	#ifdef INSTRUMENTATION
		uint32_t _ticks;						// total ticks received
		uint16_t _lowestSp;						// lowest stack pointer position seen
	#endif

	private:
		static Thread* createIdleThread();		// creates the system idle Thread

		// configures the current Thread object so that it can execute
		void configureThread(const char* name, uint8_t* stack, const uint16_t stackSize, const uint8_t quantumOverride, const ThreadEntryPoint entryPoint, const uint16_t flags);

		// prepares the stack of the Thread with the Thread's entry point and default register/parameter values
		void prepareStack(uint8_t* stack, const uint16_t stackSize, const bool quick);

		uint8_t* _stackBottom;					// the address of the lowest end of the stack
		uint16_t _stackSizeBytes;				// the size of the stack, in bytes
	};

}


// Funky little ATOMIC_BLOCK macro clones for context switching, implemented as single iteration
// for-loop in the vein of the equivalent macros in AVR-libc


// This function is run at the beginning of the atomic block to turn context switching off
static __inline__ uint8_t __iForbidRetVal() {
	zero::Thread::forbid();
	return 1;
}


// This is the cleanup function that run's when the for loop ends and the tmr_save variable goes out-of-scope.
// It will either resume context switching or not, based on the supplied parameter...
// If you're using ZERO_ATOMIC_RESTORESTATE, then tmr_save will be non-zero if switching was enabled on entry.
// If you're using ZERO_ATOMIC_FOREON, then tmr_save will have been forced to 1 at the start of the for loop
static __inline__ void __iZeroRestore(const uint8_t* __tmr_save) {
	if (*__tmr_save) {
		zero::Thread::permit();
	}
}


// The macros themselves that implement the atomic block
#define ZERO_ATOMIC_BLOCK(t)		for ( t, __ToDo = __iForbidRetVal(); __ToDo ; __ToDo = 0 )
#define ZERO_ATOMIC_RESTORESTATE	uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t)(zero::Thread::isSwitchingEnabled())
#define ZERO_ATOMIC_FORCEON			uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t) 1)


// macro to implement delay() based on ::waitUntil()
#define delay(ms) { Thread::me()->waitUntil(Thread::now() + (ms)); }


#endif
