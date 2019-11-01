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

	const int TIMESLICE_MS = 35;

	typedef int (*ThreadEntryPoint)();

	enum ThreadState {
		RUNNING,
		READY,
		PAUSED,
		TERMINATED,
		WAIT_TERM,
		WAIT_READ,
		WAIT_WRITE,
	};

	// Thread class
	class Thread {
	public:
		// kickstarts the scheduler
		static void init();

		// general blocking and unblocking
		static void block(const ThreadState newState, const uint32_t blockInfo);
		static void unblock(const ThreadState state, const uint32_t blockInfo);

		// for applications to find the current Thread
		static Thread* me();

		// Thread creation
		Thread(const char*, const uint16_t stackSize, const ThreadEntryPoint entryPoint);
		static Thread* create(const uint16_t stackSize, const ThreadEntryPoint entryPoint);
		bool setParameter(const uint8_t parameterNumber, const uint16_t value);

		// Thread management
		bool run(bool willJoin);
		int join();
		bool remove();
		bool cleanup();

		// Housekeeping
		void setName(const char* name);

		// Properties
		uint16_t getStackBottom();
		uint16_t getStackTop();
		uint16_t getStackSize();

		uint16_t calcCurrentStackBytesUsed();
		uint16_t calcPeakStackBytesUsed();

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

		bool _willJoin;
		ThreadState _state;
		int (*_entryPoint)();
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
#define thread(v,sz,fn)											\
	const PROGMEM char _threadName_##v[] = "/threads/" #v;		\
	zero::Thread v(_threadName_##v,sz,[]()fn)

// Funky little ATOMIC_BLOCK macro clones for context switching
static __inline__ uint8_t __iPermitRetVal() {
	TIMSK1 &= ~(1 << OCIE1A);
	return 1;
}

static __inline__ void __iZeroRestore(const uint8_t* __tmr_save) {
	TIMSK1 |= *__tmr_save;
}

static __inline__ void __iZeroForceOn(const uint8_t* __tmr_save) {
	TIMSK1 |= (1 << OCIE1A);
}

extern uint8_t __iPermitRetVal();

#define ZERO_ATOMIC_BLOCK(t) for ( t, __ToDo = __iPermitRetVal(); __ToDo ; __ToDo = 0 )
#define ZERO_ATOMIC_RESTORESTATE uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t)(TIMSK1 & (1 << OCIE1A))
#define ZERO_ATOMIC_FORCEON uint8_t tmr_save __attribute__((__cleanup__(__iZeroForceOn))) = (uint8_t)(TIMSK1 & (1 << OCIE1A))

}

#endif
