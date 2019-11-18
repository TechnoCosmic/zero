/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "zero_config.h"
#include "thread_macros.h"
#include "thread.h"
#include "atomic.h"
#include "list.h"
#include "string.h"
#include "memory.h"
#include "util.h"

using namespace zero;

static const int REGISTER_COUNT = 32;
static const int PC_COUNT = 2;

static const PROGMEM char DEFAULT_THREAD_NAME[] = "noname";
static const PROGMEM char _idleThreadName[] = "idle";

static inline void saveCurrentContext(Thread* destThread) __attribute__((always_inline));
static inline void restoreContext(Thread* t) __attribute__((always_inline));
static inline Thread* selectNextThread() __attribute__((always_inline));
static inline void yield() __attribute__((always_inline));

static List<Thread> _threadList;
static Thread* _currentThread = 0UL;
static Thread* _idleThread = 0UL;
static uint16_t _nextTid = 1;
static volatile bool _ctxEnabled = false;
static volatile uint64_t _milliseconds = 0ULL;


// Removes the Thread from the scheduler. This does NOT
// remove the Thread from the list of system objects.
bool Thread::remove() {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {		
		if (_state != TS_TERMINATED) {
			return false;
		}
		_threadList.remove(this);
		return true;
	}
}


// Frees the Thread's memory and removes it from the system objects list
bool Thread::cleanup() {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		if (_state != TS_TERMINATED) {
			return false;
		}

		NamedObject::remove((NamedObject*) this);
		memory::deallocate(_stackBottom, _stackSizeBytes);

		_stackBottom = 0UL;
		_stackSizeBytes = 0;

		delete this;
		return true;
	}
}


// all threads start in here, so we can clean up after them easily
static void globalThreadEntry(Thread* t) {
	// run the thread, and capture it's return code
	t->_exitCode = t->_entryPoint();

	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		// mark it as terminated
		t->_state = TS_TERMINATED;
	
		// remove from the list of running threads
		t->remove();
	
		// tidy up if it's an auto cleanup job...
		if (t->_launchFlags & TLF_AUTO_CLEANUP) {
			t->cleanup();
	
		} else {
			// ... or unblock anyone waiting to join()
			Thread::unblock(TS_JOINING, (uint32_t) t);
		}
		
		// clear this so that context won't be needlessly saved
		_currentThread = 0UL;
	}

	// this is the official end of a Thread,
	// so time to schedule another one
	yield();
}


// Configure a chunk of memory so it can be used as a stack for a Thread
void Thread::prepareStack(uint8_t* stack, const uint16_t stackSize, const bool erase) {
	uint8_t* stackEnd = &stack[stackSize-1];

	if (erase) {
		// clear the stack space in case it's recycled memory
		memset(stack, 0, stackSize);
	}

	// the entry point for the Thread
	stackEnd[ 0] = ((uint32_t) globalThreadEntry) & 0xFF;
	stackEnd[-1] = ((uint32_t) globalThreadEntry) >> 8;

	// set the stack pointer to the right place
	_sp = (uint16_t) &stackEnd[-(REGISTER_COUNT + PC_COUNT)];

	// pass ourselves in as a parameter to the launch function
	setParameter(0, (uint16_t) this);
}


// configure the Thread object ready for the execution of a new Thread
void Thread::configureThread(const char* name, uint8_t* stack, const uint16_t stackSize, const uint8_t quantumOverride, const ThreadEntryPoint entryPoint, const uint16_t flags) {
	// prepare the stack and registers
	Thread::prepareStack(stack, stackSize, !(flags & TLF_QUICK));

	_sreg = 0;
#ifdef RAMPZ
	_rampz = 0;
#endif

	// so that globalThreadEntry knows what to call
	_entryPoint = entryPoint;

	// stack details
	_stackBottom = stack;
	_stackSizeBytes = stackSize;

	// set up system data
	_tid = _nextTid++;
	_systemData._objectType = THREAD;
	_systemData._objectName = TOT(name, DEFAULT_THREAD_NAME);

	// initial housekeeping data
	if (flags & TLF_READY) {
		_state = TS_READY;
	} else {
		_state = TS_PAUSED;
	}

	_quantumTicks = TOT(quantumOverride, TIMESLICE_MS);
	_launchFlags = flags;
	_blockInfo = 0UL;

#ifdef INSTRUMENTATION
	_ticks = 0UL;
	_lowestSp = _sp;
#endif
}

// creates the idle thread, for running when nothing else wants to
Thread* Thread::createIdleThread() {
	return new Thread(_idleThreadName, 0, 5, [](){ while (1); return 0; }, TLF_READY | TLF_AUTO_CLEANUP);
}


// abridged ctor for easier threads
Thread::Thread(const uint16_t stackSizeBytes, const ThreadEntryPoint entryPoint, const int flags) : Thread(0UL, stackSizeBytes, 0, entryPoint, flags) { }


// main ctor
Thread::Thread(const char* name, const uint16_t stackSizeBytes, const uint8_t quantumOverride, const ThreadEntryPoint entryPoint, const int flags) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

		uint16_t allocated = 0UL;
		uint16_t requestedStackBytes = MAX(stackSizeBytes, THREAD_MIN_STACK_BYTES);
		uint8_t* stackBottom = (uint8_t*) memory::allocate(requestedStackBytes, &allocated, THREAD_MEMORY_SEARCH_STRATEGY);

		configureThread(name, stackBottom, allocated, quantumOverride, entryPoint, flags);

		// add ourselves to the list of system objects
		NamedObject::add((NamedObject*) this);

		// and to the thread list
		_threadList.append(this);
	}
}


#define REG_FOR_PARAM(p) (24-((p)*2))
#define OFFSET_FOR_REG(r) ((r)+1)

bool Thread::setParameter(const uint8_t parameterNumber, const uint16_t v) {
	if (parameterNumber >= 0 && parameterNumber <= 8) {
		const int8_t offset = OFFSET_FOR_REG(REG_FOR_PARAM(parameterNumber));
		((uint8_t*)0)[_sp+offset+0] = v & 0xFF;
		((uint8_t*)0)[_sp+offset+1] = v >> 8;

		return true;
	}

	return false;
}

// so that the millisecond timer automatically
// scales with your chosen MCU speed
#define SCALE(x) ((F_CPU * (x)) / 16000000UL)

// Initializes and starts the pre-emptive scheduler

// NOTE: Timer2 is used here, because on most ATmega MCUs, that's an
// 8-bit timer with a /128 pre-scalar option. This pre-scalar is used
// because it has the resolution to scale up and down with the chosen
// clock speed, without needing to use a 16-bit timer.
// Feel free to relocate this to another timer if you need to. Just
// make sure it ticks every millisecond at your chosen MCU frequency.

void Thread::init() {
	// 8-bit Timer/Counter2
	power_timer2_enable();
	TCNT2 = 0;											// reset counter to 0
	TCCR2A = (1 << WGM21);								// CTC
	TCCR2B = (1 << CS22) | (1 << CS20);					// /128 prescalar
	OCR2A = SCALE(125UL)-1;								// 1ms
	TIMSK2 |= (1 << OCIE2A);							// enable ISR

	// set up the idle Thread
	_idleThread = Thread::createIdleThread();
	NamedObject::add((NamedObject*) _idleThread);

	// enable ISR switching at the Timer level
	Thread::permit();									// enable context switching
}


// Who am I?
Thread* Thread::me() {
	return _currentThread;
}


// Prevents context switching
void Thread::forbid() {
	_ctxEnabled = false;
}


// Enables context switching
void Thread::permit() {
	_ctxEnabled = true;
}


// Determines if context switching is on
bool Thread::isSwitchingEnabled() {
	return _ctxEnabled;
}


// Saves the current state of the MCU to _currentThread
static inline void saveCurrentContext(Thread* destThread) {
	SAVE_REGS;
	if (destThread) {
		destThread->_sreg = SREG;
		destThread->_sp = SP;
#ifdef RAMPZ
		destThread->_rampz = RAMPZ;
#endif

		// switch to the 'kernel' stack
		SP = RAMEND;

#ifdef INSTRUMENTATION
		destThread->_lowestSp = MIN(destThread->_lowestSp, destThread->_sp);
#endif
	}

	// reschedule the thread to run again if it simply ran out of time
	if (destThread) {
		if (destThread->_state == TS_RUNNING) {
			destThread->_state = TS_READY;
		}
	}
}


// Restores the context of the supplied Thread and resumes it's execution
static inline void restoreContext(Thread* t) {
	_currentThread = t;
	_currentThread->_remainingTicks = _currentThread->_quantumTicks;
	_currentThread->_state = TS_RUNNING;

#ifdef RAMPZ
	RAMPZ = _currentThread->_rampz;
#endif
	SP = _currentThread->_sp;
	SREG = _currentThread->_sreg;
	RESTORE_REGS;
}


// halts the calling thread and transfers MCU control
// over to another Thread of the scheduler's choosing
static inline void yield() {
	saveCurrentContext(_currentThread);
	TIMSK2 &= ~(1 << OCIE2B);					// switch off context switch ISR
	restoreContext(selectNextThread());
}


// main decision maker for the round-robin scheduler
static inline Thread* getNextThread(Thread* firstToCheck) {
	const uint32_t now = Thread::now();
	Thread* rc = 0UL;
	Thread* cur = firstToCheck;
	Thread* startedAt = firstToCheck;

	// while we have something to check...
	while (cur) {
		// if it's ready to run, we're done!
		if (cur->_state == TS_READY) {
			rc = cur;
			break;
		}

		if (cur->_state == TS_WAITING && now >= cur->_blockInfo) {
			rc = cur;
			break;
		}

		// otherwise get the next Thread
		cur = cur->_next;

		// if we got to the end of the list,
		// start again at the head
		if (!cur) {
			cur = _threadList.getHead();
		}

		// if we've come full circle, escape,
		// we didn't find anything to run
		if (cur == startedAt) {
			break;
		}
	}

	// return either the Thread we found,
	// or the idle thread if nothing was found
	return TOT(rc, _idleThread);
}


static inline Thread* selectNextThread() {
	Thread* firstToCheck = TTT(_currentThread, _currentThread->_next);

	if (!firstToCheck || firstToCheck == _idleThread) {
		firstToCheck = _threadList.getHead();
	}

	return getNextThread(firstToCheck);
}


void Thread::waitUntil(const uint32_t untilMs) {
	// Threads can only sleep themselves, they can't be forced
	if (_currentThread != this) return;
	Thread::block(TS_WAITING, untilMs);
}


// Starts a PAUSED Thread executing by allowing it to be scheduled.
// Returns false if the Thread was not PAUSED at the time of the call.
bool Thread::run() {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		
		if (_state != TS_PAUSED) {
			return false;
		}

		this->_state = TS_READY;
		return true;
	}
}


// Pauses a running/ready Thread. Blocks if the caller is pausing itself.
// Returns false if the Thread was not RUNNING/READY at the time of the call.
bool Thread::pause() {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		
		if (_state != TS_RUNNING && _state != TS_READY && _state != TS_WAITING) {
			return false;
		}
		this->_state = TS_PAUSED;
	}

	// block immediately if the thread paused itself
	if (this == _currentThread) {
		yield();
	}

	return true;
}


// Blocks the *calling* Thread until *this* Thread terminates.
// NOTE: autoCleanup against *this* Thread must be set to FALSE prior
// to *this* Thread terminating for the join to be successful.
int Thread::join() {
	int rc = -1;

	// join can only occur on Threads
	// that do NOT auto-cleanup
	if (!(_launchFlags & TLF_AUTO_CLEANUP)) {

		// block the *calling* Thread until *this* Thread terminates
		if (_state != TS_TERMINATED) {
			block(TS_JOINING, (uint32_t) this);
		}

		// the main Thread entry code deposits the
		// Thread's exit code into the TCB for us
		// to return here
		rc = this->_exitCode;

		// cleanup the Thread properly now
		if (!cleanup()) {
			rc = -1;
		}
	}
	
	return rc;
}


// Blocks the calling Thread, setting a new state and blockInfo
void Thread::block(const ThreadState newState, uint32_t blockInfo) {
	cli();
	_currentThread->_state = newState;
	_currentThread->_blockInfo = blockInfo;
	yield();
}


// Unblock any/all Threads matching a given state and blockInfo
void Thread::unblock(const ThreadState state, const uint32_t blockInfo) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		Thread* cur = _threadList.getHead();

		while (cur) {
			Thread* next = cur->_next;

			if (cur->_state == state && cur->_blockInfo == blockInfo) {
				cur->_state = TS_READY;
				cur->_blockInfo = 0UL;
			}
			cur = next;
		}
	}
}


// triggers Timer2 COMPB ISR, which will do the switch
static void triggerContextSwitch() {
	if (Thread::isSwitchingEnabled()) {
		OCR2B = TCNT2;
		TIMSK2 |= (1 << OCIE2B);
	}
}


// millisecond timer ISR
ISR(TIMER2_COMPA_vect) {
	cli();
	_milliseconds++;

#ifdef INSTRUMENTATION
	_currentThread->_ticks++;
	_currentThread->_lowestSp = MIN(_currentThread->_lowestSp, SP);
#endif

	if (Thread::isSwitchingEnabled()) {
		// subtract from the time remaining in the quantum
		if (_currentThread->_remainingTicks > 0) {
			_currentThread->_remainingTicks--;
		}

		// if the Thread is OUTATIME...
		if (_currentThread->_remainingTicks == 0) {
			triggerContextSwitch();
		}
	}

	// we don't have to re-enable ISRs here, because
	// this *is* an ISR, meaning it will finish with
	// 'reti', which will re-enable ISRs for us
}


// context switch ISR
ISR(TIMER2_COMPB_vect, ISR_NAKED) {
	yield();
}


// returns the number of milliseconds uptime since the scheduler started
uint64_t Thread::now() {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		return _milliseconds;
	}
}


extern void startup_sequence();

// int main() __attribute__((__naked__));

// normal main() to start the system off
int main() {
	// disable all modules, and let the
	// appropriate init routines power
	// up things as they get used
	power_all_disable();

	// prep the scheduler/ms timer
	Thread::init();

	// new entry point to zero programs
	startup_sequence();

	// and immediately cause a context switch.
	// ISRs will be enabled at the very end of
	// this first switch
	yield();
}
