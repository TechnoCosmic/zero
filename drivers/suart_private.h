//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
void onTick();

private:
    SuartTx( const SuartTx& s ) = delete;
    void operator=( const SuartTx& s ) = delete;

    bool getNextTxByte( uint8_t& data );
    void startTxTimer() const;
    void stopTxTimer() const;

    // buffer-level stuff
    uint8_t* _txBuffer{ nullptr };
    uint16_t _txBytesRemaining{ 0U };
    Synapse* _txReadySyn{ nullptr };

    // sub-byte management
    uint16_t _txReg{ 0U };

    // GPIO and comms
    uint32_t _baud{ 0UL };
    const Gpio* _gpio{ nullptr };
