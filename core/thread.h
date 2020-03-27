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

    // class decl because chicken/egg
    class Synapse;
    
    typedef int ( *ThreadEntry )();

    enum class ThreadStatus {
        Ready = 0,
        Running,
        Waiting,
        Stopped,
    };

    typedef uint16_t SignalBitField;
    typedef uint16_t ThreadFlags;

    const ThreadFlags TF_NONE{ 0 };
    const ThreadFlags TF_READY{ 1 << 0 };
    const ThreadFlags TF_POOL_THREAD{ 1 << 1 };

    // reserved signals
    const auto NUM_RESERVED_SIGS = 3;
    const SignalBitField SIG_TIMEOUT{ 1 << 0 };
    const SignalBitField SIG_START{ 1 << 1 };
    const SignalBitField SIG_STOP{ 1 << 2 };
    const SignalBitField SIG_ALL_RESERVED = SIG_TIMEOUT | SIG_START | SIG_STOP;

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
        
        // constructor
        Thread(
            const char* const name,                     // name of the Thread (pointer to Flash, not SRAM)
            const uint16_t stackSize,                   // size of the stack, in bytes
            const ThreadEntry entry,                    // the Thread's entry function
            const ThreadFlags flags = TF_READY,         // Optional flags
            const Synapse* const termSyn = nullptr,     // Synapse to signal when Thread terminates
            int* const exitCode = nullptr );            // Place to put Thread's return code

        // validity checking in the absence of exceptions
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
        uint16_t getPeakStackUsageBytes() const;

        SignalBitField getAllocatedSignals( const bool userOnly = false ) const;
        SignalBitField getCurrentSignals() const;
        SignalBitField clearSignals( const SignalBitField sigs );

        void delay( const Duration dur );
        SignalBitField wait( const SignalBitField sigs, const Duration timeout = 0_ms );
        void signal( const SignalBitField sigs );

        // Don't touch! That means you! :)
        #include "thread_private.h"
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
