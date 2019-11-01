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
#include "list.h"
#include "string.h"
#include "memory.h"
#include "util.h"

using namespace zero;

const int REGISTER_COUNT = 32;
const int PC_COUNT = 2;

static inline void saveCurrentContext() __attribute__((always_inline));
static inline void restoreNewContext(Thread* t) __attribute__((always_inline));
static inline Thread* selectNextThread() __attribute__((always_inline));
static inline void yield_internal() __attribute__((always_inline));

static const PROGMEM char DEFAULT_NAME[] = "/threads/noname";

static uint16_t _originalSp;
static List<Thread> _readyList;
static Thread* _currentThread = 0UL;
static Thread* _idleThread = 0UL;

// Removes the Thread from the scheduler. This does NOT
// remove the Thread from the list of system objects.
bool Thread::remove() {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {		
		if (_state != ThreadState::TERMINATED) {
			return false;
		}

		_readyList.remove(this);

		return true;
	}
}

// Frees the Thread's memory and removes it from the system objects list
bool Thread::cleanup() {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		if (_state != ThreadState::TERMINATED) {
			return false;
		}

		NamedObject::remove((NamedObject*) this);
		memory::deallocate(_stackBottom, _stackSize);

		_stackBottom = 0UL;
		_stackSize = 0;

		return true;
	}
}

volatile int _trc;

void terminate() {
	// grab the 16-bit int return code from the thread
	asm volatile (
		"		cli					\n"
		"		sts	_trc, r24		\n"
		"		sts	_trc+1, r25		\n"
	);
	_currentThread->_exitCode = _trc;

	// mark as terminated
	_currentThread->_state = ThreadState::TERMINATED;

	// delink from threads list
	_currentThread->remove();

	// unblock anyone waiting for that Thread to terminate
	Thread::unblock(ThreadState::WAIT_TERM, (uint32_t) _currentThread);

	// full cleanup if this thread won't be join()ed
	if (!_currentThread->_willJoin) {
		if (_currentThread->cleanup()) {
			// clear this so that the scheduler won't attempt to
			// save any context for us, since we're terminating
			_currentThread = 0UL;

		} else {
			// TODO: logging code to explain the cleanup error
		}
	}

	// yield so as to immediately cease execution of this Thread
	yield_internal();
}

// Configure a chunk of memory so it can be used as a stack for a Thread
uint16_t Thread::prepareStack(uint8_t* stack, const uint16_t stackSize, const ThreadEntryPoint entryPoint) {
	uint8_t* stackEnd = &stack[stackSize-1];

	// clear the stack space in case it's recycled memory
	memset(stack, 0, stackSize);

	// the exit handler for when the Thread terminates
	stackEnd[ 0] = ((uint16_t) terminate) & 0xFF;
	stackEnd[-1] = ((uint16_t) terminate) >> 8;

	// the entry point for the Thread
	stackEnd[-2] = ((uint16_t) entryPoint) & 0xFF;
	stackEnd[-3] = ((uint16_t) entryPoint) >> 8;

	return (uint16_t) &stackEnd[-(REGISTER_COUNT + (PC_COUNT * 2))];
}

// configure the Thread object ready for the execution of a new Thread
void Thread::configureThread(const char* name, uint8_t* stack, const uint16_t stackSize, const ThreadEntryPoint entryPoint) {
	// prepare the stack and register
	_sp = Thread::prepareStack(stack, stackSize, entryPoint);
	_sreg = 0;
#ifdef RAMPZ
	_rampz = 0;
#endif

	// stack details
	_stackBottom = stack;
	_stackSize = stackSize;

	// set up system data
	_systemData._objectType = THREAD;
	_systemData._objectName = name;

	// initial housekeeping data
	_willJoin = false;					// fire-and-forget by default
	_state = ThreadState::READY;
	_blockInfo = 0UL;

#ifdef INSTRUMENTATION
	_ticks = 0UL;
	_lowestSp = _sp;
#endif
}

// ctor
Thread::Thread(const char* name, const uint16_t stackSize, const ThreadEntryPoint entryPoint) {
	uint16_t allocated = 0UL;
	uint8_t* stack = memory::allocate(stackSize, &allocated, memory::AllocationSearchDirection::TopDown);

	if (stack) {
		configureThread(name, stack, allocated, entryPoint);

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			// add ourselves to the list of system objects
			NamedObject::add((NamedObject*) this);

			// and to the thread list
			_readyList.append(this);
		}
	}
}

// creates a new Thread, with stack and TCB allocated dynamically
Thread* Thread::create(const uint16_t stackSize, const ThreadEntryPoint entryPoint) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

		uint16_t allocated = 0UL;
		uint8_t* stackBottom = memory::allocate(stackSize, &allocated, memory::AllocationSearchDirection::TopDown);
		Thread* newThread = (Thread*) stackBottom;

		if (newThread) {
			newThread->configureThread(DEFAULT_NAME, stackBottom, allocated, entryPoint);
			newThread->_state = ThreadState::PAUSED;

			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				// add ourselves to the list of system objects
				NamedObject::add((NamedObject*) newThread);

				// and to the thread list
				_readyList.append(newThread);
			}
		}

		return newThread;
	}
}


uint16_t Thread::getStackBottom() {
	return (uint16_t) _stackBottom;
}


uint16_t Thread::getStackTop() {
	return ((uint16_t) _stackBottom) + _stackSize - 1;
}


uint16_t Thread::getStackSize() {
	return _stackSize;
}


#ifdef INSTRUMENTATION

uint16_t Thread::calcCurrentStackBytesUsed() {
	return (getStackTop() - _sp);
}


uint16_t Thread::calcPeakStackBytesUsed() {
	return (getStackTop() - _lowestSp);
}

#endif


void Thread::setName(const char* newName) {
	this->_systemData._objectName = newName;
}

const char PROGMEM _idleThreadName[] = "/threads/idle";

// creates the idle thread, for running when nothing else wants to
Thread* Thread::createIdleThread() {
	uint16_t allocated = 0UL;
	uint8_t* stackBottom = memory::allocate(IDLE_THREAD_STACK_BYTES, &allocated, memory::AllocationSearchDirection::TopDown);
	Thread* newThread = (Thread*) stackBottom;

	if (newThread) {
		newThread->configureThread(_idleThreadName, stackBottom, allocated, []() {
#ifdef IDLE_BLINK
			DDRB = PORTB = (1 << PINB5);
			while (true) {
				PORTB ^= (1 << PINB5);
				_delay_ms(1000);
			}
#else
			while (true);				;
#endif
			return 0;
		});
		
		newThread->_state = ThreadState::READY;
	}
	
	return newThread;
}

bool Thread::setParameter(const int parameterNumber, const uint16_t value) {
	if (parameterNumber >= 0 && parameterNumber <= 8) {
		const int reg = 24 - (parameterNumber * 2);
		const int absRamIndex = _sp + (reg + 1);
		((uint16_t*) 0)[absRamIndex + 1] = value >> 8;
		((uint16_t*) 0)[absRamIndex + 0] = value;
	}
}

#define SCALE(x) ((F_CPU * (x)) / 16000000UL)

// initializes and starts the pre-emptive scheduler
void Thread::init() {
	// 8-bit Timer/Counter0
	power_timer0_enable();
	TCNT0 = 0;											// reset counter to 0
	TCCR0A = (1 << WGM01);								// CTC
	TCCR0B = (1 << CS01) | (1 << CS00);					// /64 prescalar
	OCR0A = SCALE(250)-1;								// 1ms
	TIMSK0 |= (1 << OCIE0A);							// enable ISR

	// 16-bit Timer/Counter1
	power_timer1_enable();
	TCNT1 = 0;											// reset counter to 0
	TCCR1A = 0;											// CTC
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);	// /64 prescalar
	OCR1A = (SCALE(250UL) * TIMESLICE_MS) - 1;			// switching counter

	// set up the idle Thread
	_idleThread = Thread::createIdleThread();
	NamedObject::add((NamedObject*) _idleThread);

	// enable switching at the Timer level
	TIMSK1 |= (1 << OCIE1A);							// enable context switching

	// preserve our SP for context switch use
	_originalSp = SP;
}

// Saves the current state of the MCU to _currentThread
static inline void saveCurrentContext() {
	SAVE_REGS;
	if (_currentThread) {
		_currentThread->_sreg = SREG;
		_currentThread->_sp = SP;
#ifdef RAMPZ
		_currentThread->_rampz = RAMPZ;
#endif

		// switch to the 'kernel' stack
		SP = _originalSp;

#ifdef INSTRUMENTATION
		_currentThread->_lowestSp = MIN(_currentThread->_lowestSp, _currentThread->_sp);
#endif
	}

	// reschedule the thread to run again if it simply ran out of time
	if (_currentThread) {
		if (_currentThread->_state == ThreadState::RUNNING) {
			_currentThread->_state = ThreadState::READY;
		}
	}
}

// Restores the context of the supplied Thread and resumes it's execution
static inline void restoreNewContext(Thread* t) {
	_currentThread = t;
	_currentThread->_state = ThreadState::RUNNING;

	// reset the timer so that it gets a full quantum
	TCNT1 = 0;
#ifdef RAMPZ
	RAMPZ = _currentThread->_rampz;
#endif
	SP = _currentThread->_sp;
	SREG = _currentThread->_sreg;
	RESTORE_REGS;
}

static inline void yield_internal() {
	saveCurrentContext();

	Thread* t = selectNextThread();

	restoreNewContext(t);
}

static inline Thread* getNextThread(Thread* firstToCheck) {
	Thread* rc = 0UL;
	Thread* cur = firstToCheck;
	Thread* startedAt = firstToCheck;

	// otherwise, find something to run
	while (cur) {
		if (cur->_state == ThreadState::READY) {
			rc = cur;
			break;
		}

		cur = cur->_next;

		if (!cur) {
			cur = _readyList.getHead();
		}

		// if we've come full circle, escape
		if (cur == startedAt) {
			break;
		}
	}
	return TOT(rc, _idleThread);
}

static inline Thread* selectNextThread() {
	Thread* firstToCheck = _currentThread->_next;

	if (!firstToCheck || firstToCheck == _idleThread) {
		firstToCheck = _readyList.getHead();
	}

	return getNextThread(firstToCheck);
}

// Starts a PAUSED Thread executing by allowing it to be scheduled.
// Returns false if the Thread was not PAUSED at the time of the call.
bool Thread::run(bool willJoin) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		
		if (_state != ThreadState::PAUSED) {
			return false;
		}

		this->_state = ThreadState::READY;
		this->_willJoin = willJoin;

		return true;
	}
}

// Blocks the *calling* Thread until *this* Thread terminates.
// NOTE: willJoin against *this* Thread must be set to true prior
// to *this* Thread terminating for the join to be successful.
int Thread::join() {
	int rc = -1;

	if (_willJoin) {
		if (_state != ThreadState::TERMINATED) {
			block(ThreadState::WAIT_TERM, (uint32_t) this);
		}

		rc = this->_exitCode;
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
	yield_internal();
}

// Unblock any/all Threads matching a given state and blockInfo
void Thread::unblock(const ThreadState state, const uint32_t blockInfo) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		Thread* cur = _readyList.getHead();

		while (cur) {
			Thread* next = cur->_next;

			if (cur->_state == state && cur->_blockInfo == blockInfo) {
				cur->_state = ThreadState::READY;
				cur->_blockInfo = 0UL;
			}
			cur = next;
		}
	}
}

Thread* Thread::me() {
	return _currentThread;
}

// context switch ISR
ISR(TIMER1_COMPA_vect, ISR_NAKED) {
	yield_internal();
}

volatile uint32_t _milliseconds = 0UL;

// millisecond timer ISR
ISR(TIMER0_COMPA_vect) {
	cli();
	_milliseconds++;

#ifdef INSTRUMENTATION
	if (_currentThread) {
		_currentThread->_ticks++;
	}
#endif

	// don't have to re-enable ISRs here, because
	// this *is* an ISR, meaning it will finish with
	// 'reti', which will re-enable ISRs for us
}

// returns the number of milliseconds uptime since the scheduler started
uint32_t Thread::now() {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		return _milliseconds;
	}
}

// normal main() to start the system off
int main() {
	Thread::init();
	sei();
	while(1);
}
