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


#define ALIGNED(x) __attribute__((__aligned__((x))))


namespace zero {

	namespace memory {

		// allocate a contiguous chunk of memory
		void* allocate(
			const uint16_t bytesReqd,
			uint16_t* const allocatedBytes = nullptr,
			const SearchStrategy strategy = memory::SearchStrategy::BottomUp);

		// deallocate a contiguous chunk of memory
		void free(const void* const address, const uint16_t numBytes);

	}

}


#endif
