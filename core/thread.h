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

    typedef int (*ThreadEntry)();

    typedef uint16_t SignalField;
    typedef uint16_t ThreadFlags;

    const ThreadFlags TF_NONE = 0;
    const ThreadFlags TF_READY = (1L << 0);
    const ThreadFlags TF_SELF_DESTRUCT = (1L << 1);
    const ThreadFlags TF_FIRE_FORGET = TF_READY | TF_SELF_DESTRUCT;
    
    // reserved signals
    const auto RESERVED_SIGS = 1;
    const SignalField SIG_TIMEOUT = (1L << 0);

    // Thread class
    class Thread {
    public:
        // Meta
        static Thread& getCurrentThread();              // Returns the current Thread
        static uint32_t now();                          // Elapsed milliseconds since boot

        static void forbid();                           // Disable context switching
        static void permit();                           // Enable context switching
        static bool isSwitchingEnabled();               // Determines if switching is on

        // constructor
        Thread(
            const uint16_t stackSize,                   // size of the stack, in bytes
            const ThreadEntry entry,                    // the Thread's entry function
            const ThreadFlags flags = TF_FIRE_FORGET,   // Optional flags
            const SignalField termSigs = 0UL,           // Signal to set when Thread dies
            uint16_t* exitCode = nullptr);              // Place to put Thread's return code
        
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


#define me Thread::getCurrentThread()


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


// Funky little ATOMIC_BLOCK macro clone for allocating and freeing Signals
static __inline__ void __iSignalsRestore(const zero::SignalField* const __signalsToFree) {
    if (*__signalsToFree) {
        zero::Thread::getCurrentThread().freeSignals(*__signalsToFree);
    }
}


#define ZERO_SIGNAL(n)      for ( SignalField n __attribute__((__cleanup__(__iSignalsRestore))) = \
                                (SignalField)(zero::Thread::getCurrentThread().allocateSignal()), \
                                __ToDo = 1; __ToDo ; __ToDo = 0 )


#endif
