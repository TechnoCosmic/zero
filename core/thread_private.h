//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    ~Thread();

    uint16_t _sp;
    uint16_t _lowSp;

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
    uint8_t* const _stackBottom;
    uint16_t _stackSize;

    SignalBitField _allocatedSignals;
    SignalBitField _waitingSignals;
    SignalBitField _currentSignals;

    uint16_t _id{ 0 };
    const char* _name{ nullptr };
