//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_SRAM_H
#define TCRI_ZERO_SRAM_H


#include <stdint.h>
#include "thread.h"


namespace zero {


    enum class SpiXferMode {
        Tx,
        Rx,
        Exchange,
    };


    class SpiMemory {
    public:
        SpiMemory(
            const uint32_t capacityBytes,               // how many bytes does the chip hold?
            volatile uint8_t* csDdr,                    // DDR for the CS for the chip
            volatile uint8_t* csPort,                   // PORT for CS
            const uint8_t csPin,                        // pin number for CS
            Synapse readySyn);                          // Synapse to fire when ready to transfer

        void write(const void* src, const uint32_t destAddress, const uint32_t numBytes);
        void read(void* dest, const uint32_t srcAddr, const uint32_t numBytes);

        #include "sram_private.h"
    };


}


#endif