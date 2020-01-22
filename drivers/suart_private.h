//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    bool getNextTxByte(uint8_t& data);
    void startTxTimer();
    void stopTxTimer();
    void onTick();

    // buffer-level stuff
    uint8_t* _txBuffer = nullptr;
    uint16_t _txBytesRemaining = 0UL;
    Synapse _txReadySyn;

    // sub-byte management
    uint16_t _txReg = 0UL;

    // GPIO and comms
    uint32_t _baud = 0ULL;
    volatile uint8_t* _ddr = nullptr;
    volatile uint8_t* _port = nullptr;
    uint8_t _pinMask = 0;
