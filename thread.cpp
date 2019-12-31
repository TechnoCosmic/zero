//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/power.h>

#include <util/delay.h>
#include <util/atomic.h>

#include "thread.h"
#include "memory.h"
#include "list.h"
#include "util.h"


using namespace zero;


#define INLINE __attribute__((always_inline))
#define NAKED __attribute__((__naked__))


// Ready list helpers, to make list accessing and swapping easy and QUICK
#define ACTIVE_LIST_NUM     (_activeListNum)
#define EXPIRED_LIST_NUM    (_activeListNum ^ 1)
#define SWAP_LISTS          _activeListNum ^= 1;

#define ACTIVE_LIST         _readyLists[ACTIVE_LIST_NUM]
#define EXPIRED_LIST        _readyLists[EXPIRED_LIST_NUM]


// 'naked' means 'no save/restore of regs' but it will still
// put the caller's PC on the stack because it's not inline,
// and that is crucial for yield() to work correctly
static void NAKED yield();

// main() is naked because we don't care for the setup upon
// entry, and we use reti at the end of main() to start everything
int NAKED main();

// these ones are inline because we specifically don't want
// any stack/register shenanigans because that's what these
// functions are here to do, but in our own controlled way
static void INLINE saveInitialContext();
static void INLINE saveExtendedContext();
static void INLINE restoreInitialContext();
static void INLINE restoreExtendedContext();


namespace {

    // globals
    List<Thread> _readyLists[2];             // the threads that will run
    Thread* _currentThread = 0UL;            // the currently executing thread
    Thread* _idleThread = 0UL;               // to run when there's nothing else to do, and only then
    volatile uint8_t _activeListNum = 0;     // which of the two ready lists are we using as the active list?
    volatile uint64_t _ms = 0;               // elapsed milliseconds
    volatile bool _switchingEnabled = true;  // context switching ISR enabled?

    // constants
    const uint8_t SIGNAL_BITS = sizeof(SignalField) * 8;
    const uint16_t REGISTER_COUNT = 32;

    #ifdef RAMPZ
        const uint16_t EXTRAS_COUNT = 2;
    #else
        const uint16_t EXTRAS_COUNT = 1;
    #endif

    // the offsets from the stack top (as seen AFTER all the registers have been pushed onto the stack already)
    // of each of the nine (9) parameters that are register-passed by GCC
    const PROGMEM uint8_t _paramOffsets[] = { 24, 26, 28, 30, 2, 4, 6, 8, 10 };


    // Determine where in the stack the registers are for a given parameter number
    // NOTE: This is GCC-specific. Different compilers may pass parameters differently.
    int getOffsetForParameter(const uint8_t parameterNumber)
    {
        if (parameterNumber < 9) {
            return pgm_read_byte((uint16_t) _paramOffsets + parameterNumber);
        }

        return 0;
    }


    // All threads start life here
    void globalThreadEntry(
        Thread& t,
        const uint32_t entry,
        const ThreadFlags flags,
        Synapse notifySyn,
        uint16_t* exitCode)
    {
        // run the thread and get its exit code
        uint16_t ec = ((ThreadEntry) entry)();

        // we don't want to be disturbed while cleaning up
        cli();

        // return the exit code if the parent wants it
        if (exitCode) {
            *exitCode = ec;
        }

        // if the parent wanted to be signalled upon
        // this Thread's termination, signal them
        notifySyn.signal();

        // remove from the list of Threads
        ACTIVE_LIST.remove(t);

        // forget us so that no context is remembered
        // superfluously in the yield() below
        _currentThread = 0UL;

        // tidy up, maybe
        if (flags & TF_SELF_DESTRUCT) {
            // Like garbage collection, this means the Thread
            // wants us to deallocate everything. The stack
            // will be deallocated in the Thread's dtor
            delete &t;
        }

        // NEXT!
        yield();
    }


    // Chooses the next Thread to run. This is the head of the active list,
    // unless there are no Threads ready to run, in which case this will
    // choose the idle Thread.
    Thread* selectNextThread()
    {
        Thread* rc = ACTIVE_LIST.getHead();

        if (!rc) {
            SWAP_LISTS;
            rc = ACTIVE_LIST.getHead();

            if (!rc) {
                rc = _idleThread;
            }
        }

        return rc;
    }


    // zero's heartbeat
    void initTimer0()
    {
        #define SCALE(x) (( F_CPU * (x)) / 16'000'000ULL)

        // 8-bit Timer/Counter0
        power_timer0_enable();      // switch it on
        TCCR0B = 0;                 // stop the clock
        TCNT0 = 0;                  // reset counter to 0
        TCCR0A = (1 << WGM01);      // CTC
        TCCR0B = (1 << CS02);       // /256 prescalar
        OCR0A = SCALE(63U)-1;       // 1ms
        TIMSK0 |= (1 << OCIE0A);    // enable ISR
    }

}


// Returns the currently executing Thread
Thread& Thread::getCurrentThread()
{
    return *_currentThread;
}


// Returns the elapse milliseconds since startup
uint64_t Thread::now()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        return _ms;
    }
}


// Prevent context switching
void Thread::forbid()
{
    _switchingEnabled = false;
}


// Enable context switching
void Thread::permit()
{
    _switchingEnabled = true;
}


// Determines if context switching is on or not
bool Thread::isSwitchingEnabled()
{
    return _switchingEnabled;
}


// ctor
Thread::Thread(
    const uint16_t stackSize,
    const ThreadEntry entry,
    const ThreadFlags flags,
    const SignalField termSigs,
    uint16_t* exitCode)
{    
    // allocate a stack from the heap
    _stackBottom = memory::allocate(MAX(stackSize, 96), &_stackSize, memory::SearchStrategy::TopDown);

    const uint16_t stackTop = (uint16_t) _stackBottom + _stackSize - 1;
    const uint16_t newStackTop = stackTop - (PC_COUNT + REGISTER_COUNT + EXTRAS_COUNT);

    // little helper for stack manipulation - yes, we're
    // going to deliberately index through a null pointer!
    #define SRAM ((uint8_t*) 0)

    // clear the stack
    for (auto i = (uint16_t) _stackBottom; i <= stackTop; i++) {
        SRAM[i] = 0;
    }
    
    // 'push' the program counter onto the stack
    SRAM[stackTop - 0] = (((uint32_t) globalThreadEntry) >>  0) & 0xFF;
    SRAM[stackTop - 1] = (((uint32_t) globalThreadEntry) >>  8) & 0xFF;

#if PC_COUNT >= 3
    SRAM[stackTop - 2] = (((uint32_t) globalThreadEntry) >> 16) & 0xFF;
#endif

#if PC_COUNT >= 4
    SRAM[stackTop - 3] = (((uint32_t) globalThreadEntry) >> 24) & 0xFF;
#endif

    // set the Thread object into parameter 0 (first parameter)
    SRAM[newStackTop + getOffsetForParameter(0) - 0] = (((uint16_t) this) >> 0) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(0) - 1] = (((uint16_t) this) >> 8) & 0xFF;

    // set the real entry point into parameters 2/1 - this will be called by globalThreadEntry()
    SRAM[newStackTop + getOffsetForParameter(2) - 0] = (((uint32_t) entry) >>  0) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(2) - 1] = (((uint32_t) entry) >>  8) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(1) - 0] = (((uint32_t) entry) >> 16) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(1) - 1] = (((uint32_t) entry) >> 24) & 0xFF;

    // set the flags
    SRAM[newStackTop + getOffsetForParameter(3) - 0] = (((uint16_t) flags) >> 0) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(3) - 1] = (((uint16_t) flags) >> 8) & 0xFF;

    // set the Synapse for notifying the parent of the Thread's termination
    SRAM[newStackTop + getOffsetForParameter(5) - 0] = (((uint16_t) _currentThread) >> 0) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(5) - 1] = (((uint16_t) _currentThread) >> 8) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(4) - 0] = (((SignalField) termSigs) >> 0) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(4) - 1] = (((SignalField) termSigs) >> 8) & 0xFF;

    // set the place for the exit code
    SRAM[newStackTop + getOffsetForParameter(6) - 0] = (((uint16_t) exitCode) >> 0) & 0xFF;
    SRAM[newStackTop + getOffsetForParameter(6) - 1] = (((uint16_t) exitCode) >> 8) & 0xFF;

    // The prepared stack has all the registers + SREG + RAMPZ 'pushed'
    // onto it (zeroed out). So we need to set a new SP that reflects
    // the 32 registers + SREG/RAMPZ + PC pushed onto it.
    // The reason is because the way that a thread is launched ultimately
    // ends with a call to restoreInitialContext(), which pops all these
    // null values off the stack and into the registers proper
    _sp = newStackTop;

    // Signal defaults
    _allocatedSignals = 0UL;
    _waitingSignals = 0UL;
    _currentSignals = 0UL;

    // ready to run?
    if (flags & TF_READY) {
        // add the Thread into the ready list
        ACTIVE_LIST.append(*this);
    }
}


// dtor
Thread::~Thread()
{
    // deallocate the stack
    memory::free(_stackBottom, _stackSize);
    _stackBottom = 0UL;
    _stackSize = 0UL;
}


// Moves the Thread into the expired list
void Thread::expire()
{
    ACTIVE_LIST.remove(*this);
    EXPIRED_LIST.append(*this);
}


// Saves enough of the register set that we can do some basic things inside a naked ISR.
static void inline saveInitialContext()
{
    asm volatile ("push r0");

    asm volatile ("in r0, __SREG__");               // status register
    asm volatile ("push r0");
#ifdef RAMPZ
    asm volatile ("in r0, __RAMPZ__");              // RAMPZ
    asm volatile ("push r0");
#endif
    asm volatile ("push r1");
    asm volatile ("push r18");
    asm volatile ("push r19");
    asm volatile ("push r20");
    asm volatile ("push r21");
    asm volatile ("push r22");
    asm volatile ("push r23");
    asm volatile ("push r24");
    asm volatile ("push r25");
    asm volatile ("push r26");
    asm volatile ("push r27");
    asm volatile ("push r28");
    asm volatile ("push r29");
}


// Save the remainder of the register set that wasn't saved by saveInitialContext()
static void inline saveExtendedContext()
{
    asm volatile ("push r30");
    asm volatile ("push r31");
    asm volatile ("push r2");
    asm volatile ("push r3");
    asm volatile ("push r4");
    asm volatile ("push r5");
    asm volatile ("push r6");
    asm volatile ("push r7");
    asm volatile ("push r8");
    asm volatile ("push r9");
    asm volatile ("push r10");
    asm volatile ("push r11");
    asm volatile ("push r12");
    asm volatile ("push r13");
    asm volatile ("push r14");
    asm volatile ("push r15");
    asm volatile ("push r16");
    asm volatile ("push r17");
}


// Restores the basic minimal register set
static void inline restoreInitialContext()
{
    asm volatile ("pop r29");
    asm volatile ("pop r28");
    asm volatile ("pop r27");
    asm volatile ("pop r26");
    asm volatile ("pop r25");
    asm volatile ("pop r24");
    asm volatile ("pop r23");
    asm volatile ("pop r22");
    asm volatile ("pop r21");
    asm volatile ("pop r20");
    asm volatile ("pop r19");
    asm volatile ("pop r18");
    asm volatile ("pop r1");
#ifdef RAMPZ
    asm volatile ("pop r0");
    asm volatile ("out __RAMPZ__, r0");
#endif
    asm volatile ("pop r0");
    asm volatile ("out __SREG__, r0");

    asm volatile ("pop r0");
}


// Restores the extended register set
static void inline restoreExtendedContext()
{
    asm volatile ("pop r17");
    asm volatile ("pop r16");
    asm volatile ("pop r15");
    asm volatile ("pop r14");
    asm volatile ("pop r13");
    asm volatile ("pop r12");
    asm volatile ("pop r11");
    asm volatile ("pop r10");
    asm volatile ("pop r9");
    asm volatile ("pop r8");
    asm volatile ("pop r7");
    asm volatile ("pop r6");
    asm volatile ("pop r5");
    asm volatile ("pop r4");
    asm volatile ("pop r3");
    asm volatile ("pop r2");
    asm volatile ("pop r31");
    asm volatile ("pop r30");
}


// Voluntarily hands control of the MCU over to another thread. Called by wait().
static void yield()
{
    // DND
    cli();

    if (_currentThread) {
        // save current context for when we unblock
        saveInitialContext();
        saveExtendedContext();
        _currentThread->_sp = SP;

        // take it out of the running
        ACTIVE_LIST.remove(*_currentThread);
    }

    // kernel stack
    SP = RAMEND;

    // select the next thread to run
    _currentThread = selectNextThread();

    // restore it's context
    SP = _currentThread->_sp;
    restoreExtendedContext();
    restoreInitialContext();

    // and off we go
    reti();
}


// The Timer tick - the main heartbeat
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    // save what we need in order to do basic stuff (non ctx switching)
    saveInitialContext();

    // increase the ms counter
    // TODO: Change this to inline assembly using only the 'initial' register set
    _ms++;

    // let's figure out switching
    if (_currentThread) {
        // only subtract time if there's time to subtract
        if (_currentThread->_ticksRemaining) {
            _currentThread->_ticksRemaining--;
        }

        // if the Thread has more time to run, or switching is disabled, bail
        if (_currentThread->_ticksRemaining || !_switchingEnabled) {
            restoreInitialContext();

            // return, enabling interrupts
            reti();
        }

        // we're switching, so we need to save the rest of the context
        saveExtendedContext();
        _currentThread->_sp = SP;

        // send it to the expired list
        if (_currentThread != _idleThread) {
            _currentThread->expire();
        }
    }

    // kernel stack
    SP = RAMEND;

    // choose the next thread
    _currentThread = selectNextThread();

    // top up the Thread's quantum if it has none left
    if (!_currentThread->_ticksRemaining) {
        _currentThread->_ticksRemaining = QUANTUM_TICKS;
    }

    // bring the new thread online
    SP = _currentThread->_sp;

    // and of course we need to do a full restore because context switch
    restoreExtendedContext();
    restoreInitialContext();

    // return, enabling interrupts
    reti();
}


// Attempts to allocate a specific Signal number
bool Thread::tryAllocateSignal(const uint16_t signalNumber)
{
    if (signalNumber >= SIGNAL_BITS) return false;

    const SignalField m = 1L << signalNumber;

    if (!(_allocatedSignals & m)) {
        _allocatedSignals |= m;
        return true;
    }

    return false;
}


// Tries to find an unused Signal number and then allocates it for use.
// If you supply a specific Signal number, only that Signal will be
// allocated, and only if it is currently free. Supplying -1 here
// will let the kernel find a free Signal number for you.
SignalField Thread::allocateSignal(const uint16_t reqdSignalNumber)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (reqdSignalNumber < SIGNAL_BITS) {
            if (tryAllocateSignal(reqdSignalNumber)) {
                return 1L << reqdSignalNumber;
            }
    
        } else {
            for (auto i = 0; i < SIGNAL_BITS; i++) {
                if (tryAllocateSignal(i)) {
                    return 1L << i;
                }
            }
        }
    }

    return 0;
}


// Frees a Signal number and allows its re-use by the Thread
void Thread::freeSignals(const SignalField signals)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _allocatedSignals &= ~signals;
        _waitingSignals &= ~signals;
        _currentSignals &= ~signals;
    }
}


// Returns a SignalField showing which Signals are currently active
SignalField Thread::getActiveSignals()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        return (_currentSignals & _waitingSignals & _allocatedSignals);
    }
}


// Returns a SignalField showing which Signals are currently active
SignalField Thread::getCurrentSignals()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        return _currentSignals;
    }
}


// Clears a set of Signals and returns the remaining ones
SignalField Thread::clearSignals(const SignalField sigs)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        return (_currentSignals &= ~sigs);
    }
}


// Waits for any of a set of Signals, returning a SignalField
// representing the Signals that woke the Thread up again
SignalField Thread::wait(const SignalField sigs)
{
    SignalField rc = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {    
        // A Thread can wait only on its own Signals.
        if (_currentThread != this) return 0;

        // set which Signals we are waiting on
        _waitingSignals = (sigs & _allocatedSignals);

        // if we're not going to end up waiting on anything, bail
        if (!_waitingSignals) return 0;

        // see what Signals are already set that we care about
        rc = getActiveSignals();

        // if there aren't any, block to wait for them
        if (!rc) {
            // this will block until at least one Signal is
            // received that we are waiting for. Execution
            // will resume immediately following the yield()
            yield();

            // disable ISRs again so we're back to being atomic
            cli();

            // Figure out which Signal(s) woke us
            rc = getActiveSignals();
        }

        // clear the recd Signals so that we can see repeats of them
        clearSignals(rc);

        // return the Signals that woke us
        return rc;
    }
}


// Send Signals to a Thread, potentially waking it up
void Thread::signal(const SignalField sigs)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const bool alreadySignalled = getActiveSignals();

        _currentSignals |= (sigs & _allocatedSignals);

        // if this resulted in the Thread waking up,
        // schedule it to run next
        if (!alreadySignalled && getActiveSignals()) {
            // remove it, in case it's already there
            ACTIVE_LIST.remove(*this);

            // add it to the top of the list, so that it's next up
            ACTIVE_LIST.prepend(*this);

            // if the Thread just signalled isn't the one running
            // now, then let's quickly get that Thread running
            if (_currentThread != this) {
                _currentThread->_ticksRemaining = 1;
            }
        }
    }
}


// Kickstart the system
int main()
{
    // startup_sequence is the developer-supplied main() replacement
    extern void startup_sequence();

    // idleThreadEntry is the developer-supplied "do nothing" idle thread
    extern int idleThreadEntry();
    
    // start Timer0 (does not enable global ints)
    initTimer0();

    // create the idle Thread
    _idleThread = new Thread(0, idleThreadEntry, TF_NONE);

    // bootstrap
    startup_sequence();

    // bring the first thread on-line
    _currentThread = selectNextThread();

    // top up its quantum
    _currentThread->_ticksRemaining = QUANTUM_TICKS;

    // set the current stack pointer to first thread's
    SP = _currentThread->_sp;

    // restore the (brand new) context for the chosen thread
    restoreExtendedContext();
    restoreInitialContext();

    // this enables global interrupts as well as returning
    reti();
}
