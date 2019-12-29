//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


private:
    uint8_t* _buffer = 0UL;
    uint16_t _bufferSize = 0UL;
    uint16_t _pivot = 0UL;
    uint16_t _writeOffset = 0UL;
    uint16_t _usedBytes = 0UL;
