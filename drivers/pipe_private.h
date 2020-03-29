//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    /// @privatesection
    ~Pipe();

private:
    Pipe( const Pipe& p ) = delete;
    void operator=( const Pipe& p ) = delete;

    uint8_t* const _buffer{ nullptr };
    uint16_t _bufferSize;
    uint16_t _startIndex{ 0 };
    uint16_t _length{ 0 };

    Synapse* _roomAvailSyn{ nullptr };
    Synapse* _dataAvailSyn{ nullptr };

    PipeFilter _readFilter{ nullptr };
    PipeFilter _writeFilter{ nullptr };
