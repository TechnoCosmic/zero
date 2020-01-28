//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    bool getNextTxByte(uint8_t& data);
    void byteTxComplete();
    
    uint8_t _deviceNum = 0;
    uint8_t* _txBuffer = nullptr;
    uint16_t _txBytesRemaining = 0UL;
    Synapse* _txReadySyn;
