//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_PAGEMANAGER_H
#define TCRI_ZERO_PAGEMANAGER_H


#include <stdint.h>


#define ROUND_UP(v,r) ((((v)-1)|((r)-1))+1)


const uint16_t SRAM_PAGES = DYNAMIC_BYTES / PAGE_BYTES;


namespace zero {

    namespace memory {

        enum SearchStrategy {
            TopDown = 0,
            BottomUp,
        };
    }

    template <uint16_t PAGE_COUNT>
    class PageManager {
    public:
        // low-level functions
        bool isPageAvailable(const uint16_t pageNumber);
        void markAsFree(const uint16_t pageNumber);
        void markAsUsed(const uint16_t pageNumber);

        uint16_t getTotalPageCount();
        uint16_t getUsedPageCount();
        uint16_t getFreePageCount();

        // higher level operations
        int16_t findFreePages(const uint16_t numPagesRequired, const memory::SearchStrategy strat);

    private:
        uint8_t _memoryMap[ROUND_UP(PAGE_COUNT, 8) / 8];
    };

}

#endif
