//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    /// @privatesection
    ~UsartRx();
    static void onRx( const uint8_t deviceNum, const uint8_t data );

    Synapse* _rxDataReceivedSyn{ nullptr };
    Synapse* _rxOverflowSyn{ nullptr };
    DoubleBuffer* _rxBuffer{ nullptr };

private:
    UsartRx( const UsartRx& u ) = delete;
    void operator=( const UsartRx& u ) = delete;

    uint8_t _deviceNum = 0;
