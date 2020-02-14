//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


private:
    Synapse( const Synapse& s ) = delete;
    void operator=( const Synapse& s ) = delete;

    Thread* const _thread;
    const SignalField _signals;
