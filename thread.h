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


static const uint8_t THREAD_MIN_STACK_BYTES = 48;

namespace zero {

	typedef int (*ThreadEntryPoint)();

	enum ThreadState {
		TS_RUNNING,
		TS_READY,
		TS_PAUSED,
		TS_TERMINATED,
		TS_WAIT_TERM,
		TS_WAIT_ATOMIC_WRITE,
		TS_WAIT_READ,
		TS_WAIT_WRITE,
	};

	const int TLF_READY = 1;
	const int TLF_AUTO_CLEANUP = 2;

	// Thread class
	class Thread {
	public:
		// kickstarts the scheduler
		static void init();

		// Thread creation
		Thread(const char* name, const uint16_t stackSize, const uint8_t quantumOverride, const ThreadEntryPoint entryPoint, const int flags);

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
		bool run();
		bool pause();
		int join();
		bool remove();
		bool cleanup();

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
		uint8_t _quantumMs;
		uint8_t _remainingTicks;
		int (*_entryPoint)();
		bool _autoCleanup;
		int _exitCode;

	#ifdef INSTRUMENTATION
		uint32_t _ticks;
		uint16_t _lowestSp;
	#endif

	private:
		void configureThread(const char* name, uint8_t* stack, const uint16_t stackSize, const uint8_t quantumOverride, const ThreadEntryPoint entryPoint, const int flags);
		static uint16_t prepareStack(uint8_t* stack, const uint16_t stackSize);
		static Thread* createIdleThread();

		uint32_t _blockInfo;
		uint8_t* _stackBottom;
		uint16_t _stackSize;
	};

}


#endif
