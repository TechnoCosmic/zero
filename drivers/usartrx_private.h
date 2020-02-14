//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    Synapse* _rxDataReceivedSyn = nullptr;
    Synapse* _rxOverflowSyn = nullptr;
    DoubleBuffer* _rxBuffer = nullptr;

private:
    uint8_t _deviceNum = 0;

    UsartRx( const UsartRx& u ) = delete;
    void operator=( const UsartRx& u ) = delete;
