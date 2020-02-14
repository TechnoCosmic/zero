//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    // higher level operations
    int16_t findFreePages(
        const uint16_t numPagesRequired,
        const memory::SearchStrategy strat ) const;

private:
    uint8_t _memoryMap[ ROUND_UP( PAGE_COUNT, 8 ) / 8 ];
