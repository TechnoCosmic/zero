//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


private:
    Adc( const Adc& s ) = delete;
    void operator=( const Adc& s ) = delete;

    const Synapse& _readySyn;
    uint16_t _lastConversion;
