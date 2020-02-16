//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_THREAD_H
#define TCRI_ZERO_THREAD_H


#include <stdint.h>


namespace zero {

    // class decl because chicken/egg
    class Synapse;
    
    typedef int ( *ThreadEntry )();

    typedef uint16_t SignalField;
    typedef uint16_t ThreadFlags;

    const ThreadFlags TF_NONE = 0;
    const ThreadFlags TF_READY = ( 1 << 0 );

    // reserved signals
    const auto RESERVED_SIGS = 1;
    const SignalField SIG_TIMEOUT = ( 1 << 0 );
    const SignalField SIG_ALL_RESERVED = SIG_TIMEOUT;

    // Thread class
    class Thread {
    public:
        // meta
        static Thread& getCurrent();                    // Returns the current Thread
        static uint32_t now();                          // Elapsed milliseconds since boot

        static void forbid();                           // Disable context switching
        static void permit();                           // Enable context switching
        static bool isSwitchingEnabled();               // Determines if switching is on

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

        // Stack information
        uint16_t getPeakStackUsage() const;

        SignalField getAllocatedSignals( const bool userOnly = false ) const;
        SignalField getCurrentSignals() const;
        SignalField clearSignals( const SignalField sigs );

        void delay( const uint32_t ms );
        SignalField wait( const SignalField sigs, const uint32_t timeoutMs = 0UL );
        void signal( const SignalField sigs );

        // Don't touch! That means you! :)
        #include "thread_private.h"
    };

}    // namespace zero


#define me Thread::getCurrent()


// Funky little ATOMIC_BLOCK macro clones for context switching
static __inline__ uint8_t __iForbidRetVal()
{
    zero::Thread::forbid();
    return 1;
}


static __inline__ void __iZeroRestore( const uint8_t* const __tmr_save )
{
    if ( *__tmr_save ) {
        zero::Thread::permit();
    }
}


#define ZERO_ATOMIC_BLOCK( t )      for ( t, __ToDo = __iForbidRetVal(); __ToDo; __ToDo = 0 )
#define ZERO_ATOMIC_FORCEON         uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t) 1)
#define ZERO_ATOMIC_RESTORESTATE    uint8_t tmr_save __attribute__( ( __cleanup__( __iZeroRestore ) ) ) = ( uint8_t )( zero::Thread::isSwitchingEnabled() )


#endif
