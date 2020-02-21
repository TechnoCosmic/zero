//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_PAGEMANAGER_H
#define TCRI_ZERO_PAGEMANAGER_H


#include <stdint.h>
#include <avr/io.h>


#define ROUND_UP( v, r )        ( ( ( (v) -1 ) | ( (r) -1 ) ) + 1 )


const uint16_t SRAM_PAGES{ DYNAMIC_BYTES / PAGE_BYTES };


namespace zero {

    namespace memory {

        enum class SearchStrategy {
            TopDown = 0,
            BottomUp,
        };
    }

    template <uint16_t PAGE_COUNT>
    class PageManager {
    public:
        // low-level functions
        bool isPageAvailable( const uint16_t pageNumber ) const;
        void markAsFree( const uint16_t pageNumber );
        void markAsUsed( const uint16_t pageNumber );

        uint16_t getTotalPageCount() const;
        uint16_t getUsedPageCount() const;
        uint16_t getFreePageCount() const;

        #include "pagemanager_private.h"
    };

}    // namespace zero

#endif
