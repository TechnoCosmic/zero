/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_PAGEMANAGER_H
#define TCRI_ZERO_PAGEMANAGER_H

#include <stdint.h>
#include "util.h"

namespace zero {

    template <uint16_t PAGE_COUNT>
    class PageManager {
    public:
        bool isPageAvailable(const uint16_t pageNumber);
        void markAsFree(const uint16_t pageNumber);
        void markAsUsed(const uint16_t pageNumber);

        uint16_t getPageCount();

    private:
        uint8_t _memoryMap[ROUND_UP(PAGE_COUNT, 8) / 8];
    };

}

#endif



