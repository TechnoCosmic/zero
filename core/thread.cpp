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
#include <avr/sleep.h>

#include <util/delay.h>
#include <util/atomic.h>

#include "thread.h"
#include "synapse.h"
#include "resource.h"
#include "memory.h"
#include "power.h"
#include "debug.h"
#include "gpio.h"
#include "list.h"
#include "time.h"
#include "util.h"
#include "attrs.h"


using namespace zero;


// Ready list helpers, to make list accessing and swapping easy and QUICK
#define ACTIVE_LIST_NUM     ( _activeListNum )
#define EXPIRED_LIST_NUM    ( _activeListNum ^ 1 )
#define SWAP_LISTS          _activeListNum ^= 1;

#define ACTIVE_LIST         _readyLists[ ACTIVE_LIST_NUM ]
#define EXPIRED_LIST        _readyLists[ EXPIRED_LIST_NUM ]


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
    List<Thread> _readyLists[ 2 ];                      // the Threads that will run
    List<Thread> _poolThreadList;                       // the Threads waiting for code to run
    OffsetList<Thread> _timeoutList;                    // the list of Threads wanting to sleep for a time
    Thread* _currentThread{ nullptr };                  // the currently executing thread
    Thread* _idleThread{ nullptr };                     // to run when there's nothing else to do, and only then
    uint16_t _nextId{ 0 };                              // ID to use for the next Thread
    volatile uint8_t _activeListNum{ 0 };               // which of the two ready lists are we using as the active list?
    volatile uint32_t _milliseconds{ 0UL };             // 49 day millisecond counter
    volatile bool _switchingEnabled{ true };            // context switching ISR enabled?

    // constants
    const uint8_t SIGNAL_BITS{ sizeof( SignalBitField ) * 8 };
    const uint16_t REGISTER_COUNT{ 32 };

#ifdef RAMPZ
    const uint16_t EXTRAS_COUNT{ 2 };
#else
    const uint16_t EXTRAS_COUNT{ 1 };
#endif

    #define MIN_STACK_BYTES 128


    // the offsets from the stack top (as seen AFTER all the registers have been pushed onto
    // the stack already) of each of the nine (9) parameters that are register-passed by GCC
    constexpr uint8_t _paramOffsets[] = { 24, 26, 28, 30, 2, 4, 6, 8, 10 };

    // Determine where in the stack the registers are for a given parameter number
    // NOTE: This is GCC-specific. Different compilers may pass parameters differently.
    constexpr int getOffsetForParameter( const uint8_t parameterNumber )
    {
        if ( parameterNumber < 9 ) {
            return _paramOffsets[ parameterNumber ];
        }

        return 0;
    }


    // Chooses the next Thread to run. This is the head of the active list,
    // unless there are no Threads ready to run, in which case this will
    // choose the idle Thread.
    Thread* selectNextThread()
    {
        Thread* rc{ ACTIVE_LIST.getHead() };

        if ( !rc ) {
            SWAP_LISTS;
            rc = ACTIVE_LIST.getHead();

            if ( !rc ) {
                rc = _idleThread;
            }
        }

        return rc;
    }


    uint16_t getNewThreadId()
    {
        ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
            return _nextId++;
        }
    }


    // zero's heartbeat
    void initTimer0()
    {
        #define SCALE( x )      ( ( F_CPU_MHZ * ( x ) ) / 16U )

        #ifndef TIMSK0
            #define TIMSK0 TIMSK
        #endif

        static_assert( QUANTUM_TICKS > 1, "QUANTUM_TICKS must be two (2) or more" );
        static_assert( F_CPU >=  4'000'000, "Must use a 4MHz clock or faster" );
        static_assert( F_CPU <= 24'000'000, "Must use a 24MHz clock or slower" );

        // 8-bit Timer/Counter0
        power_timer0_enable();                          // switch it on
        TCCR0B = 0;                                     // stop the clock
        TCNT0 = 0;                                      // reset counter to 0
        TCCR0A = ( 1 << WGM01 );                        // CTC
        TCCR0B = ( 1 << CS02 );                         // /256 prescalar

        OCR0A = SCALE( 62.5 ) - 1;                      // 1ms
        TIMSK0 |= ( 1 << OCIE0A );                      // enable ISR

        OCR0B = SCALE( 62.5 ) - 1;                      // 1ms
        TIMSK0 |= ( 1 << OCIE0B );                      // enable ISR
    }

}    // namespace


// Default Thread exit handler
void WEAK onThrexit( Thread&, const int )
{
    // empty
}


// Default stack overflow handler
void WEAK onStackOverflow( Thread& )
{
    // empty
}


// Default idle thread
int WEAK idleThreadEntry()
{
    while ( true ) {
        Power::idle();
    }
}


static void callStackOverflowHandler()
{
    const uint16_t oldSp = SP;

    SP = RAMEND;
    onStackOverflow( *_currentThread );
    SP = oldSp;
}


// All threads start and end life here
void Thread::globalThreadEntry(
    Thread& t,
    const uint32_t entry,
    const ThreadFlags flags,
    const Synapse* const notifySyn,
    int* const exitCode )
{
    // run the thread and get its exit code
    int ec{ ( (ThreadEntry) entry )() };

    // we don't want to be disturbed while cleaning up
    cli();

    // Thread should NOT have signals still allocated,
    // other than reserved signals. This is a check to
    // ensure the Thread was well-behaved. If a Thread
    // doesn't deallocate all of it's signals, it means
    // there's a reference out there somewhere, via a
    // Synapse, to this Thread (if there wasn't, the
    // Synapses would have deallocated all the signals).
    // If there's a reference to this Thread somewhere,
    // then this Thread cannot be recycled in a Thread
    // pool, since any new Thread occupying this object
    // may be signalled in error via that old Synapse,
    // and that's bad, m'kay?
    if ( flags & TF_POOL_THREAD ) {
        dbg_assert( !t.getAllocatedSignals( true ), "Signals remain" );

        // TODO: Log the offending code, by storing its name
        // in EEPROM.
    }

    // return the exit code if someone wants it
    if ( exitCode ) {
        *exitCode = ec;
    }

    // if someone wanted to be signalled upon
    // this Thread's termination, signal them
    if ( notifySyn ) {
        notifySyn->signal();
    }

    // remove from the list of Threads
    ACTIVE_LIST.remove( t );

    // forget us so that no context is remembered
    // superfluously in the yield() below
    _currentThread = nullptr;

    // call global Thread termination handler
    onThrexit( t, ec );

    // Pool Threads get returned to the pool, and
    // other Threads get cleaned up
    if ( flags & TF_POOL_THREAD ) {
        _poolThreadList.append( t );
    }
    else {
        // The stack will be deallocated in the Thread's dtor
        delete &t;
    }

    // NEXT!
    yield();
}


// Returns the currently executing Thread
Thread& Thread::getCurrent()
{
    return *_currentThread;
}


// Returns the number of milliseconds since the MCU started.
// NOTE: Wraps around after approximately 49 continuous days
uint32_t Thread::now()
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        return _milliseconds;
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


void Thread::reanimate(
    const char* const name,                             // name of Thread, points to Flash memory
    const ThreadEntry entry,                            // the Thread's entry function
    const ThreadFlags flags,                            // Optional flags
    const Synapse* const termSyn,                       // Synapse to signal when Thread terminates
    int* const exitCode )                               // Place to put Thread's return code
{
    static_assert( PC_COUNT >= 2 and PC_COUNT <= 4, "PC_COUNT must be 2,3, or 4" );
    dbg_assert( entry, "No entry point" );

    const uint16_t stackTop{ (uint16_t) _stackBottom + _stackSize - 1 };
    const uint16_t newStackTop{ stackTop - ( PC_COUNT + REGISTER_COUNT + EXTRAS_COUNT ) };

    _id = getNewThreadId();
    _name = name;

    // little helper for stack manipulation - yes, we're
    // going to deliberately index through a null pointer!
    #define SRAM ( (uint8_t*) 0 )

    // 'push' the program counter onto the stack
    SRAM[ stackTop - 0 ] = ( ( (uint32_t) globalThreadEntry ) >> 0 ) & 0xFF;
    SRAM[ stackTop - 1 ] = ( ( (uint32_t) globalThreadEntry ) >> 8 ) & 0xFF;

#if PC_COUNT >= 3
    SRAM[ stackTop - 2 ] = ( ( (uint32_t) globalThreadEntry ) >> 16 ) & 0xFF;
#endif

#if PC_COUNT >= 4
    SRAM[ stackTop - 3 ] = ( ( (uint32_t) globalThreadEntry ) >> 24 ) & 0xFF;
#endif

    // set the Thread object into parameter 0 (first parameter)
    SRAM[ newStackTop + getOffsetForParameter( 0 ) - 0 ] = ( ( (uint16_t) this ) >> 0 ) & 0xFF;
    SRAM[ newStackTop + getOffsetForParameter( 0 ) - 1 ] = ( ( (uint16_t) this ) >> 8 ) & 0xFF;

    // set the real entry point into parameters 2/1 - this will be called by globalThreadEntry()
    SRAM[ newStackTop + getOffsetForParameter( 2 ) - 0 ] = ( ( (uint32_t) entry ) >> 0 ) & 0xFF;
    SRAM[ newStackTop + getOffsetForParameter( 2 ) - 1 ] = ( ( (uint32_t) entry ) >> 8 ) & 0xFF;
    SRAM[ newStackTop + getOffsetForParameter( 1 ) - 0 ] = ( ( (uint32_t) entry ) >> 16 ) & 0xFF;
    SRAM[ newStackTop + getOffsetForParameter( 1 ) - 1 ] = ( ( (uint32_t) entry ) >> 24 ) & 0xFF;

    // Flags
    SRAM[ newStackTop + getOffsetForParameter( 3 ) - 0 ] = ( ( (uint16_t) flags ) >> 0 ) & 0xFF;
    SRAM[ newStackTop + getOffsetForParameter( 3 ) - 1 ] = ( ( (uint16_t) flags ) >> 8 ) & 0xFF;

    // Termination Synapse
    SRAM[ newStackTop + getOffsetForParameter( 4 ) - 0 ] = ( ( (uint16_t) termSyn ) >> 0 ) & 0xFF;
    SRAM[ newStackTop + getOffsetForParameter( 4 ) - 1 ] = ( ( (uint16_t) termSyn ) >> 8 ) & 0xFF;

    // set the place for the exit code
    SRAM[ newStackTop + getOffsetForParameter( 5 ) - 0 ] = ( ( (uint16_t) exitCode ) >> 0 ) & 0xFF;
    SRAM[ newStackTop + getOffsetForParameter( 5 ) - 1 ] = ( ( (uint16_t) exitCode ) >> 8 ) & 0xFF;

    // The prepared stack has all the registers + SREG + RAMPZ 'pushed'
    // onto it (zeroed out). This new stack top represents that.
    _sp = newStackTop;
    _lowSp = _sp;

    // signal defaults
    _allocatedSignals = SIG_ALL_RESERVED;
    _waitingSignals = 0;
    _currentSignals = 0;

    // sleeping time
    _timeoutOffset = 0UL;
}


// Removes a Thread from the pool (if one is available), and re-animates it.
Thread* Thread::fromPool(
    const char* const name,                             // name of the Thread (pointer to Flash, not SRAM)
    const ThreadEntry entry,                            // the Thread's entry function
    const Synapse* const termSyn,                       // Synapse to signal when Thread terminates
    int* const exitCode )                               // Place to put Thread's return code
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        Thread* rc{ nullptr };

        if ( ( rc = _poolThreadList.getHead() ) ) {
            // make sure it doesn't get used by someone else
            _poolThreadList.remove( *rc );

            // insert new code into it
            rc->reanimate(
                name,
                entry,
                TF_READY | TF_POOL_THREAD,
                termSyn,
                exitCode );

            // make sure it gets to run
            ACTIVE_LIST.prepend( *rc );
        }

        return rc;
    }
}


// ctor
Thread::Thread(
    const char* const name,                             // name of Thread, points to Flash memory
    const uint16_t stackSize,                           // size of the stack, in bytes
    const ThreadEntry entry,                            // the Thread's entry function
    const ThreadFlags flags,                            // Optional flags
    const Synapse* const termSyn,                       // Synapse to signal when Thread terminates
    int* const exitCode )                               // Place to put Thread's return code
:
    _stackBottom{ (uint8_t*) memory::allocate(
        MAX( stackSize, MIN_STACK_BYTES ),
        &_stackSize,
        memory::SearchStrategy::TopDown ) }
{
    dbg_assert( _stackBottom and _stackSize, "No stack memory" );

    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        // Pool Threads get pooled immediately, ready for use
        if ( flags & TF_POOL_THREAD ) {
            _poolThreadList.append( *this );
        }
        else {
            // normal Threads become 'animated' immediately
            reanimate( name, entry, flags, termSyn, exitCode );

            // ready to run?
            if ( flags & TF_READY ) {
                // add the Thread into the ready list
                ACTIVE_LIST.append( *this );
            }
        }        
    }
}


// dtor
Thread::~Thread()
{
    // deallocate the stack
    memory::free( _stackBottom, _stackSize );
}


// validity checking
Thread::operator bool() const
{
    return _stackBottom;
}


// Returns the ID of the Thread
uint16_t Thread::getThreadId() const
{
    return _id;
}


// Returns the name of the Thread
const char* Thread::getName() const
{
    return _name;
}


// Returns the allocated size of the stack, in bytes
uint16_t Thread::getStackSizeBytes() const
{
    return _stackSize;
}


// Returns the peak recorded stack usage, in bytes
uint16_t Thread::getPeakStackUsageBytes() const
{
    return ( _stackSize - ( _lowSp - (uint16_t) _stackBottom ) );
}


// Saves the register set
static void inline saveInitialRegisters()
{
    asm volatile( "push r0" );

    asm volatile( "in r0, __SREG__" );
    asm volatile( "push r0" );
#ifdef RAMPZ
    asm volatile( "in r0, __RAMPZ__" );
    asm volatile( "push r0" );
#endif
    asm volatile( "push r1" );
    asm volatile( "push r18" );
    asm volatile( "push r19" );
    asm volatile( "push r20" );
    asm volatile( "push r21" );
    asm volatile( "push r22" );
    asm volatile( "push r23" );
    asm volatile( "push r24" );
    asm volatile( "push r25" );
    asm volatile( "push r26" );
    asm volatile( "push r27" );
    asm volatile( "push r28" );
    asm volatile( "push r29" );
    asm volatile( "push r30" );
    asm volatile( "push r31" );
}


// Saves the register set
static void inline saveExtendedRegisters()
{
    asm volatile( "push r2" );
    asm volatile( "push r3" );
    asm volatile( "push r4" );
    asm volatile( "push r5" );
    asm volatile( "push r6" );
    asm volatile( "push r7" );
    asm volatile( "push r8" );
    asm volatile( "push r9" );
    asm volatile( "push r10" );
    asm volatile( "push r11" );
    asm volatile( "push r12" );
    asm volatile( "push r13" );
    asm volatile( "push r14" );
    asm volatile( "push r15" );
    asm volatile( "push r16" );
    asm volatile( "push r17" );
}


// Restores the register set
static void inline restoreExtendedRegisters()
{
    asm volatile( "pop r17" );
    asm volatile( "pop r16" );
    asm volatile( "pop r15" );
    asm volatile( "pop r14" );
    asm volatile( "pop r13" );
    asm volatile( "pop r12" );
    asm volatile( "pop r11" );
    asm volatile( "pop r10" );
    asm volatile( "pop r9" );
    asm volatile( "pop r8" );
    asm volatile( "pop r7" );
    asm volatile( "pop r6" );
    asm volatile( "pop r5" );
    asm volatile( "pop r4" );
    asm volatile( "pop r3" );
    asm volatile( "pop r2" );
}


// Restores the register set
static void inline restoreInitialRegisters()
{
    asm volatile( "pop r31" );
    asm volatile( "pop r30" );
    asm volatile( "pop r29" );
    asm volatile( "pop r28" );
    asm volatile( "pop r27" );
    asm volatile( "pop r26" );
    asm volatile( "pop r25" );
    asm volatile( "pop r24" );
    asm volatile( "pop r23" );
    asm volatile( "pop r22" );
    asm volatile( "pop r21" );
    asm volatile( "pop r20" );
    asm volatile( "pop r19" );
    asm volatile( "pop r18" );
    asm volatile( "pop r1" );
#ifdef RAMPZ
    asm volatile( "pop r0" );
    asm volatile( "out __RAMPZ__, r0" );
#endif
    asm volatile( "pop r0" );
    asm volatile( "out __SREG__, r0" );

    asm volatile( "pop r0" );
}


// Voluntarily hands control of the MCU over to another thread.
// Called by wait(), globalThreadEntry(), and main().
static void yield()
{
    // DND
    cli();

    if ( _currentThread ) {
        // save current context for when we unblock
        saveInitialRegisters();
        saveExtendedRegisters();
        _currentThread->_sp = SP;

        // track stack usage at switch point
        _currentThread->_lowSp = MIN( _currentThread->_lowSp, _currentThread->_sp );

        // check for stack overflow
        if ( _currentThread->_lowSp < (uint16_t) _currentThread->_stackBottom ) {
            callStackOverflowHandler();
        }

        // take it out of the running
        ACTIVE_LIST.remove( *_currentThread );

        // see if it wanted to sleep
        if ( _currentThread->_timeoutOffset ) {
            _timeoutList.insertByOffset( *_currentThread, _currentThread->_timeoutOffset );
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
ISR( TIMER0_COMPA_vect )
{
    _milliseconds++;

    // check sleepers
    if ( Thread* curSleeper = _timeoutList.getHead() ) {
        if ( curSleeper->_timeoutOffset ) {
            curSleeper->_timeoutOffset--;
        }

        while ( curSleeper and !curSleeper->_timeoutOffset ) {
            _timeoutList.remove( *curSleeper );
            curSleeper->signal( SIG_TIMEOUT );

            curSleeper = _timeoutList.getHead();
        }
    }
}


// Pre-emptive context switch
ISR( TIMER0_COMPB_vect, ISR_NAKED )
{
    // save registers enough to do basic checking
    saveInitialRegisters();

    // let's figure out switching
    if ( _currentThread ) {
        // only subtract time if there's time to subtract
        if ( _currentThread->_ticksRemaining ) {
            _currentThread->_ticksRemaining--;
        }

        // faux priorities - if we're not the head of the active list, then
        // a switch is required so that we run the current head instead
        if ( _switchingEnabled and _currentThread != ACTIVE_LIST.getHead() ) {
            _currentThread->_ticksRemaining = 0;
        }

        // if the Thread has more time to run, or switching is disabled, bail
        if ( _currentThread->_ticksRemaining or !_switchingEnabled ) {
            // strategic goto to save undue additional
            // expansion of inline restoreInitialRegisters()
            goto exit;
        }

        // we're switching, so we need to save the rest
        saveExtendedRegisters();
        _currentThread->_sp = SP;

        // track peak stack usage at switch point
        _currentThread->_lowSp = MIN( _currentThread->_lowSp, _currentThread->_sp );

        if ( _currentThread->_lowSp < (uint16_t) _currentThread->_stackBottom ) {
            callStackOverflowHandler();
        }

        // send it to the expired list
        if ( _currentThread != _idleThread ) {
            ACTIVE_LIST.remove( *_currentThread );
            EXPIRED_LIST.append( *_currentThread );
        }
    }

    // choose the next thread
    _currentThread = selectNextThread();

    // top up the Thread's quantum if it has none left
    if ( !_currentThread->_ticksRemaining ) {
        _currentThread->_ticksRemaining = QUANTUM_TICKS;
    }

    // bring the new thread online
    SP = _currentThread->_sp;
    restoreExtendedRegisters();

exit:

    restoreInitialRegisters();
    reti();
}


// Attempts to allocate a specific signal number
bool Thread::tryAllocateSignal( const uint16_t signalNumber )
{
    if ( signalNumber >= SIGNAL_BITS ) return false;

    const SignalBitField m{ 1U << signalNumber };

    if ( !( _allocatedSignals & m ) ) {
        _allocatedSignals |= m;
        return true;
    }

    return false;
}


// Tries to find an unused signal number, and then allocates it for use.
// If you supply a specific signal number, only that signal will be
// allocated, and only if it is currently free. Supplying -1 here
// will let the kernel find a free signal number for you.
SignalBitField Thread::allocateSignal( const uint16_t reqdSignalNumber )
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        if ( reqdSignalNumber < SIGNAL_BITS ) {
            if ( tryAllocateSignal( reqdSignalNumber ) ) {
                return 1U << reqdSignalNumber;
            }
        }
        else {
            // start checking after the reserved signals, for speed
            for ( auto i = NUM_RESERVED_SIGS; i < SIGNAL_BITS; i++ ) {
                if ( tryAllocateSignal( i ) ) {
                    return 1U << i;
                }
            }
        }
    }

    return 0;
}


// Frees a signal number and allows its re-use by the Thread
void Thread::freeSignals( const SignalBitField signals )
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        // can't free the reserved signals
        const SignalBitField sigsTofree{ signals & ~SIG_ALL_RESERVED };

        _allocatedSignals &= ~sigsTofree;
        _waitingSignals &= ~sigsTofree;
        _currentSignals &= ~sigsTofree;
    }
}


// Returns the currently allocated signals
SignalBitField Thread::getAllocatedSignals( const bool userOnly ) const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        if ( userOnly ) {
            return _allocatedSignals & ~SIG_ALL_RESERVED;
        }
        else {
            return _allocatedSignals;
        }
    }
}


// Returns a SignalBitField showing which signals are currently active,
// where "active" means that the Thread will wake up because of them
SignalBitField Thread::getActiveSignals() const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        return _currentSignals & _waitingSignals;
    }
}


// Returns a SignalBitField showing which signals are currently set
SignalBitField Thread::getCurrentSignals() const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        return _currentSignals;
    }
}


// Clears a set of signals and returns the remaining ones
SignalBitField Thread::clearSignals( const SignalBitField sigs )
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        return ( _currentSignals &= ~sigs );
    }
}


// Blocks for a given number of milliseconds
void Thread::delay( const Duration dur )
{
    wait( 0, dur );
}


// Waits for any of a set of signals, returning a SignalBitField
// representing which of those signals woke the Thread up
SignalBitField Thread::wait( const SignalBitField sigs, const Duration timeout )
{
    SignalBitField rc{ 0 };

    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        // A Thread can wait only on it's own signals.
        if ( _currentThread != this ) {
            return 0;
        }

        // build the final field from scratch
        _waitingSignals = sigs;

        // make sure the signal gets used if the Thread wants a timeout set
        _timeoutOffset = (uint32_t) timeout;

        if ( _timeoutOffset ) {
            _waitingSignals |= SIG_TIMEOUT;
        }
        else {
            // force the flag off, in case it was specified, but with no
            // timeoutMs value supplied
            _waitingSignals &= ~SIG_TIMEOUT;
        }

        // ensure we're only waiting on signals we have allocated
        _waitingSignals &= _allocatedSignals;

        // if we're not going to end up waiting on anything, bail
        if ( !_waitingSignals ) {
            return 0;
        }

        // see what signals are already set that we care about
        rc = getActiveSignals();

        // if there aren't any, block to wait for them
        if ( !rc ) {
            // this will block until at least one signal is
            // received that we are waiting for. Execution
            // will resume immediately following the yield()
            yield();

            // disable ISRs again so we're back to being atomic
            cli();

            // Figure out which signal(s) woke us
            rc = getActiveSignals();
        }

        // clear the recd signals so that we can see repeats of them
        clearSignals( rc );

        // make sure that the timeout is disabled
        _currentThread->_timeoutOffset = 0UL;

        // return the signals that woke us
        return rc;
    }
}


// Send signals to a Thread, potentially waking it up
void Thread::signal( const SignalBitField sigs )
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        const bool alreadySignalled{ getActiveSignals() };

        // set the signals
        _currentSignals |= ( sigs & _allocatedSignals );

        // do we need to wake the Thread?
        if ( _currentThread != this and                 // if we're not signalling ourselves and,
             !alreadySignalled and                      // this thread isn't already in the active list, and,
             getActiveSignals() )                       // it now has signals that would wake it up, ...
        {
            // ... then move it to the active list ready to run

            // if it's on the timeout list, take it off
            if ( _timeoutOffset ) {
                _timeoutList.remove( *this );
                this->_timeoutOffset = 0UL;
            }

            // put it at the top of the active list, ready to go
            ACTIVE_LIST.remove( *this );
            ACTIVE_LIST.prepend( *this );
        }
    }
}


// Creates the Thread pool
static void createPoolThreads()
{
    #define __xtxt(a) __txt(a)
    #define __txt(a) #a

    static_assert(
        POOL_THREAD_STACK_BYTES >= MIN_STACK_BYTES,
        "POOL_THREAD_STACK_BYTES must be " __xtxt(MIN_STACK_BYTES)
        " or more, but is only " __xtxt(POOL_THREAD_STACK_BYTES) );

    static_assert(
        ( NUM_POOL_THREADS * POOL_THREAD_STACK_BYTES ) < DYNAMIC_BYTES,
        "Thread pool consumes entire heap" );

    #if ( NUM_POOL_THREADS * POOL_THREAD_STACK_BYTES ) >= ( DYNAMIC_BYTES / 2 )
        #warning "Thread pool consumes over half of heap memory."
    #endif

    for ( auto i = 0; i < NUM_POOL_THREADS; i++ ) {
        Thread* poolGuy = new Thread{
            nullptr,                                    // no name yet
            POOL_THREAD_STACK_BYTES,                    // one stack size to rule them all
            nullptr,                                    // no entry point yet
            TF_POOL_THREAD,                             // flags
            nullptr,                                    // no termination Synapse yet
            nullptr };                                  // no place to put exit code yet

        dbg_assert( poolGuy, "Pool thread init fail" );

        if ( poolGuy == nullptr ) {
            break;
        }
    }
}


// Kickstart the system
void CTOR preMain()
{
    #ifdef ZERO_DRIVERS_GPIO
        // initialize the GPIO - make sure everything is tri-stated
        Gpio::init();
    #endif

    // initialize the debug serial TX first so that anything can use it
    debug::init();

    // initialize the power management stuff
    if ( !Power::init() ) {
        // failure to launch - perma-sleep now
        dbg_pgm( "onReset() failed - sleeping\r\n" );

        while ( true ) {
            Power::shutdown( true, false );
        }
    }
    else {
        // create the system Threads
        _idleThread = new Thread{ PSTR( "idle" ), 0, idleThreadEntry, TF_NONE };
        createPoolThreads();

        // claim the main timer before anyone else does
        resource::obtain( resource::ResourceId::Timer0 );
    }
}


void DTOR postMain()
{
    // start Timer0 (does not enable global ints)
    initTimer0();

    // Go!
    yield();
}
