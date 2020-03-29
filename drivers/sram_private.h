//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    /// @privatesection
    ~SpiMemory();
    void select() const;
    void deselect() const;

private:
    SpiMemory( const SpiMemory& m ) = delete;
    void operator=( const SpiMemory& m ) = delete;

    void sendAddress( const uint32_t addr ) const;
    void sendReadCommand( const uint32_t addr ) const;
    void sendWriteCommand( const uint32_t addr ) const;

    const uint32_t _capacityBytes{ 0UL };
    const Gpio* _chipSelectPin{ nullptr };
