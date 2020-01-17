//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/power.h>

#include <util/delay.h>
#include <util/atomic.h>

#include "thread.h"
#include "memory.h"
#include "debug.h"
#include "../helpers/list.h"
#include "../helpers/util.h"


using namespace zero;


#define INLINE __attribute__((always_inline))
#define NAKED __attribute__((__naked__))


// Ready list helpers, to make list accessing and swapping easy and QUICK
#define ACTIVE_LIST_NUM     (_activeListNum)
#define EXPIRED_LIST_NUM    (_activeListNum ^ 1)
#define SWAP_LISTS          _activeListNum ^= 1;

#define ACTIVE_LIST         _readyLists[ACTIVE_LIST_NUM]
#define EXPIRED_LIST        _readyLists[EXPIRED_LIST_NUM]


// main() is naked because we don't care for the setup upon
// entry, and we use reti at the end of main() to start everything
int NAKED main();
static void yield();

// these ones are inline because we specifically don't want
// any stack/register shenanigans because that's what these
// functions are here to do, but in our own controlled way
static void INLINE saveInitialRegisters();
static void INLINE saveExtendedRegisters();
static void INLINE restoreExtendedRegisters();
static void INLINE restoreInitialRegisters();


namespace {
    // globals
    List<Thread> _readyLists[2];                // the threads that will run
    List<Thread> _timeoutList;                  // the list of Threads wanting to sleep for a time
    Thread* _currentThread = 0UL;               // the currently executing thread
    Thread* _idleThread = 0UL;                  // to run when there's nothing else to do, and only then
    volatile uint8_t _activeListNum = 0;        // which of the two ready lists are we using as the active list?
    volatile uint32_t _ms = 0ULL;               // 49 day millisecond counter
    volatile bool _switchingEnabled = true;     // context switching ISR enabled?

    // constants
    const uint8_t SIGNAL_BITS = sizeof(SignalField) * 8;
    const uint16_t REGISTER_COUNT = 32;

    #ifdef RAMPZ
        const uint16_t EXTRAS_COUNT = 2;
    #else
        const uint16_t EXTRAS_COUNT = 1;
    #endif

    const uint16_t MIN_STACK_BYTES = 96;
    
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
        #define SCALE(x) (( F_CPU * (x)) / 16'000'000.0)

        #ifndef TIMSK0
        #define TIMSK0 TIMSK
        #endif

        // 8-bit Timer/Counter0
        power_timer0_enable();      // switch it on
        TCCR0B = 0;                 // stop the clock
        TCNT0 = 0;                  // reset counter to 0
        TCCR0A = (1 << WGM01);      // CTC
        TCCR0B = (1 << CS02);       // /256 prescalar

        OCR0A = SCALE(62.5)-1;      // 1ms
        TIMSK0 |= (1 << OCIE0A);    // enable ISR

        OCR0B = SCALE(62.5)-1;      // 1ms
        TIMSK0 |= (1 << OCIE0B);    // enable ISR
    }

}


// All threads start life here
static void globalThreadEntry(
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


// Returns the currently executing Thread
Thread& Thread::getCurrentThread()
{
    return *_currentThread;
}


// Returns the number of milliseconds since the MCU started.
// NOTE: Wraps around after approximately 49 continuous days
uint32_t Thread::now()
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
    _stackBottom = (uint8_t*) memory::allocate(
        MAX(stackSize, MIN_STACK_BYTES),
        &_stackSize, memory::SearchStrategy::TopDown);

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
    // onto it (zeroed out). This new stack top represents that.
    _sp = newStackTop;

    // Signal defaults
    _allocatedSignals = SIG_TIMEOUT;
    _waitingSignals = 0UL;
    _currentSignals = 0UL;

    // sleeping time
    _timeoutOffset = 0ULL;

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


// Saves the register set
static void inline saveInitialRegisters()
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
    asm volatile ("push r30");
    asm volatile ("push r31");
}

// Saves the register set
static void inline saveExtendedRegisters()
{
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


// Restores the register set
static void inline restoreExtendedRegisters()
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
}


// Restores the register set
static void inline restoreInitialRegisters()
{
    asm volatile ("pop r31");
    asm volatile ("pop r30");
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


// Voluntarily hands control of the MCU over to another thread. Called by wait().
static void yield()
{
    // DND
    cli();

    if (_currentThread) {
        // save current context for when we unblock
        saveInitialRegisters();
        saveExtendedRegisters();
        _currentThread->_sp = SP;

        // take it out of the running
        ACTIVE_LIST.remove(*_currentThread);

        // see if it wanted to sleep
        if (_currentThread->_timeoutOffset) {
            _timeoutList.insertByOffset(*_currentThread, _currentThread->_timeoutOffset);
        }
    }

    // select the next thread to run
    _currentThread = selectNextThread();

    // restore it's context
    SP = _currentThread->_sp;
    restoreExtendedRegisters();
    restoreInitialRegisters();
    reti();
}


// Millisecond timer and timeout controller
ISR(TIMER0_COMPA_vect)
{
    _ms++;

    // check sleepers
    if (Thread* curSleeper = _timeoutList.getHead()) {
        if (curSleeper->_timeoutOffset) {
            curSleeper->_timeoutOffset--;
        }
        
        while (curSleeper && !curSleeper->_timeoutOffset) {
            _timeoutList.remove(*curSleeper);
            curSleeper->signal(SIG_TIMEOUT);

            curSleeper = _timeoutList.getHead();
        }
    }
}


// Pre-emptive context switch
ISR(TIMER0_COMPB_vect, ISR_NAKED)
{
    // save registers enough to do basic checking
    saveInitialRegisters();

    // let's figure out switching
    if (_currentThread) {
        // only subtract time if there's time to subtract
        if (_currentThread->_ticksRemaining) {
            _currentThread->_ticksRemaining--;
        }

        // faux priorities - if we're not the head of the active list, then
        // a switch is required so that we run the current head instead
        if (_switchingEnabled && _currentThread != ACTIVE_LIST.getHead()) {
            _currentThread->_ticksRemaining = 0UL;
        }

        // if the Thread has more time to run, or switching is disabled, bail
        if (_currentThread->_ticksRemaining || !_switchingEnabled) {
            // strategic goto to save undue additional
            // expansion of inline restoreRegisters()
            goto exit;
        }

        // we're switching, so we need to save the rest
        saveExtendedRegisters();
        _currentThread->_sp = SP;

        // send it to the expired list
        if (_currentThread != _idleThread) {
            ACTIVE_LIST.remove(*_currentThread);
            EXPIRED_LIST.append(*_currentThread);
        }
    }

    // choose the next thread
    _currentThread = selectNextThread();

    // top up the Thread's quantum if it has none left
    if (!_currentThread->_ticksRemaining) {
        _currentThread->_ticksRemaining = QUANTUM_TICKS;
    }

    // bring the new thread online
    SP = _currentThread->_sp;
    restoreExtendedRegisters();

exit:

    restoreInitialRegisters();
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
            // start checking after the reserved signals, for speed
            for (auto i = RESERVED_SIGS; i < SIGNAL_BITS; i++) {
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
        // can't free the reserved signals
        const SignalField sigsTofree = signals & ~SIG_TIMEOUT;

        _allocatedSignals &= ~sigsTofree;
        _waitingSignals &= ~sigsTofree;
        _currentSignals &= ~sigsTofree;
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
SignalField Thread::wait(const SignalField sigs, const uint32_t timeoutMs)
{
    SignalField rc = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {    
        // A Thread can wait only on it's own Signals.
        if (_currentThread != this) return 0;

        // build the final field from scratch
        _waitingSignals = sigs;

        // make sure the signal gets used if the Thread wants a timeout set
        _timeoutOffset = timeoutMs;

        if (_timeoutOffset) {
            _waitingSignals |= SIG_TIMEOUT;

        } else {
            // force the flag off, in case it was specified, but with no
            // timeoutMs value supplied
            _waitingSignals &= ~SIG_TIMEOUT;
        }

        // ensure we're only waiting on signals we have allocated
        _waitingSignals &= _allocatedSignals;

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

        // make the the timeout is disabled
        _currentThread->_timeoutOffset = 0ULL;

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

        // - if we're not signalling ourselves and,
        // - this thread isn't already in the active list, and,
        // - it now has signals that would wake it up,
        // then move it to the active list ready to run
        if (_currentThread != this && !alreadySignalled && getActiveSignals()) {
            // if it's on the timeout list, take it off
            if (_timeoutOffset) {
                this->_timeoutOffset = 0ULL;
                _timeoutList.remove(*this);
            }

            // put it at the top of the active list, ready to go
            ACTIVE_LIST.remove(*this);
            ACTIVE_LIST.prepend(*this);
        }
    }
}


// Kickstart the system
int main()
{
    // startup_sequence is the developer-supplied main() replacement
    void startup_sequence();

    // idleThreadEntry is the developer-supplied "do nothing" idle thread
    int idleThreadEntry();
    
    // initialize the debug serial TX first so that anything can use it
    debug::init();

    // bootstrap
    startup_sequence();

    // create the idle Thread
    _idleThread = new Thread(0, idleThreadEntry, TF_NONE);

    // start Timer0 (does not enable global ints)
    initTimer0();

    // Go!
    yield();
}
