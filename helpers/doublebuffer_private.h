//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


private:
    uint8_t* _buffer{ nullptr };
    uint16_t _bufferSize{ 0U };
    uint16_t _pivot{ 0U };
    uint16_t _writeOffset{ 0U };
    uint16_t _usedBytes{ 0U };
