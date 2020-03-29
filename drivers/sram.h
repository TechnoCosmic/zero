//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_SPIMEM


#ifndef TCRI_ZERO_SRAM_H
#define TCRI_ZERO_SRAM_H


#include <stdint.h>

#include "thread.h"
#include "gpio.h"


namespace zero {


    /// @brief Provdes asynchronous SPI memory services
    class SpiMemory {
    public:
        SpiMemory(
            const uint32_t capacityBytes,               // how many bytes does the chip hold?
            const Gpio& chipSelect,                     // Gpio object for the CS line
            const Synapse& readySyn );                  // Synapse to fire when ready to transfer

        void read(
            void* dest,                                 // destination address, in local SRAM
            const uint32_t srcAddr,                     // source address for the data, in external SPI memory
            const uint32_t numBytes );                  // number of bytes to read

        void write(
            const void* src,                            // source data address, in local SRAM
            const uint32_t destAddress,                 // destination address, in external SPI memory
            const uint32_t numBytes );                  // number of the bytes to write

        explicit operator bool() const;

        #include "sram_private.h"
    };


}    // namespace zero


#endif


#endif
