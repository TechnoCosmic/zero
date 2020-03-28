//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_THREAD_H
#define TCRI_ZERO_THREAD_H


#include <stdint.h>
#include "time.h"
#include "attrs.h"


namespace zero {

    typedef int ( *ThreadEntry )();

    enum class ThreadStatus {
        /// Ready to run
        Ready = 0,

        /// Currently executing
        Running,

        /// Waiting for signals
        Waiting,

        /// Stopped, waiting to restart
        Stopped,
    };


    /// A bit field representing one or more signals.
    typedef uint16_t SignalBitField;

    /// The flags controlling a Thread's behavior.
    typedef uint16_t ThreadFlags;

    // No flags
    const ThreadFlags TF_NONE{ 0 };

    /// Indicates that the Thread is ready to execute as soon as possible
    const ThreadFlags TF_READY{ 1 << 0 };

    /// Indicates that the Thread should be put into the system Thread pool
    const ThreadFlags TF_POOL_THREAD{ 1 << 1 };


    /// @private
    /// reserved signals
    const auto NUM_RESERVED_SIGS = 3;

    /// Occurs when the wait() call times out
    const SignalBitField SIG_TIMEOUT{ 1 << 0 };

    /// Occurs when a service-oriented Thread is restarted
    const SignalBitField SIG_START{ 1 << 1 };

    /// Occurs when a service-oriented Thread is asked to stop
    const SignalBitField SIG_STOP{ 1 << 2 };

    /// @private
    /// All reserved system signals
    const SignalBitField SIG_ALL_RESERVED = SIG_TIMEOUT | SIG_START | SIG_STOP;


    // forward decl because chicken/egg
    class Thread;


    class Synapse {
    public:
        Synapse();
        ~Synapse();

        explicit operator bool() const;                 // Determines if the Synapse initialized correctly
        operator SignalBitField() const;                // Extracts the signal(s) represented by the Synapse

        void signal() const;                            // Signals the Thread
        void clearSignals() const;                      // Clears the signals from the Thread
        SignalBitField wait(
            const Duration timeout = 0_ms ) const;

    private:
        Synapse( const Synapse& s ) = delete;
        void operator=( const Synapse& s ) = delete;

        Thread* const _thread;
        const SignalBitField _signals;
    };

    
    // Thread class
    class Thread {
    public:
        // meta
        static Thread& getCurrent();                    // Returns the current Thread
        static uint32_t now();                          // Elapsed milliseconds since boot

        static void forbid();                           // Disable context switching
        static void permit();                           // Enable context switching
        static bool isSwitchingEnabled();               // Determines if switching is on

        static Thread* fromPool(
            const char* const name,                     // name of the Thread (pointer to Flash, not SRAM)
            const ThreadEntry entry,                    // the Thread's entry function
            const Synapse* const termSyn = nullptr,     // Synapse to signal when Thread terminates
            int* const exitCode = nullptr );            // Place to put Thread's return code
        
        // ctor
        Thread(
            const char* const name,                     // name of the Thread (pointer to Flash, not SRAM)
            const uint16_t stackSize,                   // size of the stack, in bytes
            const ThreadEntry entry,                    // the Thread's entry function
            const ThreadFlags flags = TF_READY,         // Optional flags
            const Synapse* const termSyn = nullptr,     // Synapse to signal when Thread terminates
            int* const exitCode = nullptr );            // Place to put Thread's return code

        // Determines if the Thread initialized correctly
        explicit operator bool() const;

        // General
        uint16_t getThreadId() const;                   // Returns the ID of the Thread
        const char* getName() const;                    // Returns the name of the Thread

        // Control
        void restart();                                 // restarts the Thread
        void stop();                                    // stops the Thread
        ThreadStatus getStatus() const;                 // gets the Thread's status

        // Stack information
        uint16_t getStackSizeBytes() const;
        uint16_t getStackPeakUsageBytes() const;

        SignalBitField getAllocatedSignals( const bool userOnly = false ) const;
        SignalBitField getCurrentSignals() const;
        SignalBitField clearSignals( const SignalBitField sigs );

        void delay( const Duration dur );
        SignalBitField wait( const SignalBitField sigs, const Duration timeout = 0_ms );
        void signal( const SignalBitField sigs );

    // Don't touch! That means you! :)
    // PRIVATE!
    public:
        /// @privatesection
        ~Thread();

        uint16_t _sp;
        uint16_t _lowSp;
        uint8_t* const _stackBottom;

        uint8_t _ticksRemaining;
        uint32_t _timeoutOffset;

        Thread* _prev;
        Thread* _next;

    private:
        friend class Synapse;

        Thread( const Thread& t ) = delete;
        void operator=( const Thread& t ) = delete;

        static void globalThreadEntry(
            Thread& t,
            const uint32_t entry,
            const ThreadFlags flags,
            const Synapse* const notifySyn,
            int* const exitCode );

        void reanimate(
            const char* const newName,                      // name of Thread, points to Flash memory
            const ThreadEntry newEntry,                     // the Thread's entry function
            const ThreadFlags newFlags,                     // Optional flags
            const Synapse* const newTermSyn,                // Synapse to signal when Thread terminates
            int* const newExitCode );                       // Place to put Thread's return code

        // Signals Management
        SignalBitField allocateSignal( const uint16_t reqdSignalNumber = -1 );
        void freeSignals( const SignalBitField signals );

        SignalBitField getActiveSignals() const;
        bool tryAllocateSignal( const uint16_t signalNumber );

        // more TCB
        uint16_t _stackSize;

        SignalBitField _allocatedSignals;
        SignalBitField _waitingSignals;
        SignalBitField _currentSignals;

        uint16_t _id{ 0 };
        const char* _name{ nullptr };
    };

}    // namespace zero


#define me Thread::getCurrent()


// Funky little ATOMIC_BLOCK macro clones for context switching
static inline uint8_t __iForbidRetVal()
{
    zero::Thread::forbid();
    return 1;
}


static inline void __iZeroRestore( const uint8_t* const __tmr_save )
{
    if ( *__tmr_save ) {
        zero::Thread::permit();
    }
}


#define ZERO_ATOMIC_BLOCK( t )      for ( t, __ToDo = __iForbidRetVal(); __ToDo; __ToDo = 0 )
#define ZERO_ATOMIC_FORCEON         uint8_t tmr_save CLEANUP( __iZeroRestore ) = (uint8_t) 1
#define ZERO_ATOMIC_RESTORESTATE    uint8_t tmr_save CLEANUP( __iZeroRestore ) = ( uint8_t )( zero::Thread::isSwitchingEnabled() )


#endif
