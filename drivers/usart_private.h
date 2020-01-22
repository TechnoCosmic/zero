//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    uint8_t _deviceNum = 0;
    DoubleBuffer* _rxBuffer = nullptr;
    Synapse _rxDataReceivedSyn;
    Synapse _rxOverflowSyn;
