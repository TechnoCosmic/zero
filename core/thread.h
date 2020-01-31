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

    typedef int (*ThreadEntry)();

    typedef uint16_t SignalField;
    typedef uint16_t ThreadFlags;

    const ThreadFlags TF_NONE = 0;
    const ThreadFlags TF_READY = (1L << 0);

    // reserved signals
    const auto RESERVED_SIGS = 1;
    const SignalField SIG_TIMEOUT = (1L << 0);

    // Thread class
    class Thread {
    public:
        // Meta
        static Thread& getCurrent();                    // Returns the current Thread
        static uint32_t now();                          // Elapsed milliseconds since boot

        static void forbid();                           // Disable context switching
        static void permit();                           // Enable context switching
        static bool isSwitchingEnabled();               // Determines if switching is on

        // constructor
        Thread(
            const uint16_t stackSize,                   // size of the stack, in bytes
            const ThreadEntry entry,                    // the Thread's entry function
            const ThreadFlags flags = TF_READY,         // Optional flags
            const Synapse* const termSyn = nullptr,     // Synapse to signal when Thread terminates
            int* exitCode = nullptr);                   // Place to put Thread's return code
        
        explicit operator bool() const;

        // Stack information
        uint16_t getPeakStackUsage() const;

        // Signals Management
        SignalField allocateSignal(const uint16_t reqdSignalNumber = -1);
        void freeSignals(const SignalField signals);

        SignalField getCurrentSignals() const;
        SignalField clearSignals(const SignalField sigs);
        
        SignalField wait(const SignalField sigs, const uint32_t timeoutMs = 0ULL);
        void signal(const SignalField sigs);

        // Don't touch! That means you! :)
        #include "thread_private.h"
    };

}


#define me Thread::getCurrent()


// Funky little ATOMIC_BLOCK macro clones for context switching
static __inline__ uint8_t __iForbidRetVal() {
    zero::Thread::forbid();
    return 1;
}


static __inline__ void __iZeroRestore(const uint8_t* const __tmr_save) {
    if (*__tmr_save) {
        zero::Thread::permit();
    }
}


#define ZERO_ATOMIC_BLOCK(t)        for ( t, __ToDo = __iForbidRetVal(); __ToDo ; __ToDo = 0 )
#define ZERO_ATOMIC_FORCEON         uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t) 1)
#define ZERO_ATOMIC_RESTORESTATE    uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = \
                                        (uint8_t)(zero::Thread::isSwitchingEnabled())


#endif
