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

        /// @brief Used to control how memory is found
        enum class SearchStrategy {
            /// Search backwards from the highest address available
            TopDown = 0,

            /// Search forwards from the lowest address available
            BottomUp,
        };
    }

    /// @brief Provides simple page-based allocation tracking services
    /// @tparam PAGE_COUNT The number of pages to be tracked by the PageManager.
    template <uint16_t PAGE_COUNT>
    class PageManager {
        
        static_assert( PAGE_COUNT > 0, "Pages for heap allocator must be more than zero (0)" );

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
