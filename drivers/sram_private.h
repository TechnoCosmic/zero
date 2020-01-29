//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    ~SpiMemory();

    // fake private
    void select() const;
    void deselect() const;

private:
    SpiMemory(const SpiMemory& m) = delete;
    void operator=(const SpiMemory& m) = delete;

    void sendAddress(const uint32_t addr) const;
    void sendReadCommand(const uint32_t addr) const;
    void sendWriteCommand(const uint32_t addr) const;

    uint32_t _capacityBytes = 0ULL;
    volatile uint8_t* _csDdr = nullptr;
    volatile uint8_t* _csPort = nullptr;
    uint8_t _csPinMask = 0;
