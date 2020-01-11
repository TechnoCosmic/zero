//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    ~Thread();
    
    uint16_t _sp;
    Thread* _prev;
    Thread* _next;
    uint8_t _ticksRemaining;
    uint32_t _timeoutOffset;

private:
    SignalField getActiveSignals();
    bool tryAllocateSignal(const uint16_t signalNumber);
    
    uint8_t* _stackBottom;
    uint16_t _stackSize;

    SignalField _allocatedSignals;
    SignalField _waitingSignals;
    SignalField _currentSignals;
