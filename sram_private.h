//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    // fake private
    void select();
    void deselect();

private:
    volatile uint8_t* _csDdr;
    volatile uint8_t* _csPort;
    uint8_t _csPinMask;
