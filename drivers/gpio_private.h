//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


private:
    inline PinField sanitize(const PinField pins) const;

    const PinField _pins;                           // pins owned by this object
