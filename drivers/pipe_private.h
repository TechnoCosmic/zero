//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


private:
    Pipe(const Pipe& p) = delete;
    void operator=(const Pipe& p) = delete;

    uint8_t* _buffer = nullptr;
    uint16_t _bufferSize = 0UL;
    uint16_t _startIndex = 0UL;
    uint16_t _length = 0UL;

    Synapse* _roomAvailSyn = nullptr;
    Synapse* _dataAvailSyn = nullptr;

    PipeFilter _readFilter = nullptr;
    PipeFilter _writeFilter = nullptr;
