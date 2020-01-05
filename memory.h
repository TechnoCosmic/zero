//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#ifndef TCRI_ZERO_MEMORY_H
#define TCRI_ZERO_MEMORY_H


#include <stdint.h>

#include "pagemanager.h"


namespace zero {

    namespace memory {

      uint8_t* allocate(const uint16_t bytesRqed, uint16_t* allocatedBytes, const SearchStrategy strategy);
      void free(const uint8_t* address, uint16_t numBytes);

    }

}

#endif
