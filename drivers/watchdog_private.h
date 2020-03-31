//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    /// @privatesection
    static void init();
    static void enable( const uint8_t dur );
    static void disable();
    ~Watchdog();

private:
    static WatchdogFlags allocateFlag();
    const WatchdogFlags _flag;
