//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    /// @privatesection
    ~DoubleBuffer();

private:
    uint8_t* const _buffer;
    uint16_t _bufferSize;
    const uint16_t _pivot;
    uint16_t _writeOffset;
    uint16_t _usedBytes;
