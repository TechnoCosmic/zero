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

    // Signals Management
    SignalField allocateSignal( const uint16_t reqdSignalNumber = -1 );
    void freeSignals( const SignalField signals );

    SignalField getActiveSignals() const;
    bool tryAllocateSignal( const uint16_t signalNumber );

    // more TCB
    uint8_t* const _stackBottom;
    uint16_t _stackSize;

    SignalField _allocatedSignals;
    SignalField _waitingSignals;
    SignalField _currentSignals;

    const uint16_t _id;
    const char* const _name;
