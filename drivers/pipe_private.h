//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


private:
    uint8_t* _buffer;
    uint16_t _bufferSize;
    uint16_t _startIndex;
    uint16_t _length;

    Synapse _roomAvailSyn;
    Synapse _dataAvailSyn;

    PipeFilter _readFilter;
    PipeFilter _writeFilter;
