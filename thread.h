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

namespace zero {

	typedef int (*ThreadEntryPoint)();

	enum ThreadState {
		RUNNING,
		READY,
		PAUSED,
		TERMINATED,
		WAIT_TERM,
		WAIT_ATOMIC_WRITE,
		WAIT_READ,
		WAIT_WRITE,
	};

	// Thread class
	class Thread {
	public:
		// kickstarts the scheduler
		static void init();

		// Thread creation
		static Thread* create(const char* name, const uint16_t stackSize, const ThreadEntryPoint entryPoint);
		Thread(const char*, const uint16_t stackSize, const ThreadEntryPoint entryPoint);

		// general blocking and unblocking
		static void block(const ThreadState newState, const uint32_t blockInfo);
		static void unblock(const ThreadState state, const uint32_t blockInfo);

		// for applications to find the current Thread
		static Thread* me();

		// Thread management
		static void forbid();
		static void permit();
		static bool isSwitchingEnabled();

		bool setParameter(const uint8_t parameterNumber, const uint16_t value);
		bool run(bool willJoin);
		int join();
		bool remove();
		bool cleanup();
		void setState(const ThreadState s);

		// Housekeeping
		void setName(const char* name);

		// Properties
		bool isDynamic();
		uint16_t getThreadId();
		uint16_t getStackBottom();
		uint16_t getStackTop();
		uint16_t getStackSizeBytes();
		uint16_t calcCurrentStackBytesUsed();

#ifdef INSTRUMENTATION
		uint16_t calcPeakStackBytesUsed();
#endif

		// miscellaneous, but sort of related!
		static uint32_t now();

		// NamedObject must be first
		NamedObject _systemData;
		Thread* _prev;
		Thread* _next;

		// registers and other rememberables
		uint8_t _sreg;
		uint16_t _sp;
	#ifdef RAMPZ
		uint8_t _rampz;
	#endif

		uint16_t _tid;
		ThreadState _state;
		uint8_t _remainingTicks;
		int (*_entryPoint)();
		bool _willJoin;
		int _exitCode;

	#ifdef INSTRUMENTATION
		uint32_t _ticks;
		uint16_t _lowestSp;
	#endif

	private:
		void configureThread(const char* name, uint8_t* stack, const uint16_t stackSize, const ThreadEntryPoint entryPoint);
		static uint16_t prepareStack(uint8_t* stack, const uint16_t stackSize);
		static Thread* createIdleThread();

		uint32_t _blockInfo;
		uint8_t* _stackBottom;
		uint16_t _stackSize;
	};


// helper macro for easier Thread creation
#define thread(v,sz,fn)								\
	const PROGMEM char _threadName_##v[] = #v;		\
	zero::Thread v(_threadName_##v,sz,[]()fn)


// Funky little ATOMIC_BLOCK macro clones for context switching
static __inline__ uint8_t __iForbidRetVal() {
	Thread::forbid();
	return 1;
}


static __inline__ void __iZeroRestore(const uint8_t* __tmr_save) {
	if (*__tmr_save) {
		Thread::permit();
	}
}


extern uint8_t __iForbidRetVal();


#define ZERO_ATOMIC_BLOCK(t) for ( t, __ToDo = __iForbidRetVal(); __ToDo ; __ToDo = 0 )
#define ZERO_ATOMIC_RESTORESTATE uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t)(Thread::isSwitchingEnabled())
#define ZERO_ATOMIC_FORCEON uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t) 1)

}

#endif
