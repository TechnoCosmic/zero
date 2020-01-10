//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    ~SpiMemory();

    // fake private
    void select();
    void deselect();

private:
    void sendAddress(const uint32_t addr);
    void sendReadCommand(const uint32_t addr);
    void sendWriteCommand(const uint32_t addr);

    uint32_t _capacityBytes;
    volatile uint8_t* _csDdr;
    volatile uint8_t* _csPort;
    uint8_t _csPinMask;
