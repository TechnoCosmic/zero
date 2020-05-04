//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_MEMORY_H
#define TCRI_ZERO_MEMORY_H


#include <stdint.h>
#include "pagemanager.h"


namespace zero {

    /// @brief Manages dynamic allocation of SRAM
    /// @details The amount of SRAM available to zero's allocator is defined in
    /// the `makefile`. Search for `DYNAMIC_BYTES`, making sure you're adjusting
    /// the correct one for your chosen target MCU.

    namespace memory {

        // allocate a contiguous chunk of memory
        void* allocate(
            const uint16_t bytesReqd,
            uint16_t* const allocatedBytes = nullptr,
            const SearchStrategy strategy = memory::SearchStrategy::BottomUp );

        // deallocate a contiguous chunk of memory
        void free( const void* const address, const uint16_t numBytes );

    }    // namespace memory

}    // namespace zero


#endif
