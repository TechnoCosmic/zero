/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_MEMORY_H
#define TCRI_ZERO_MEMORY_H

#include <stdint.h>

namespace zero {

    namespace memory {

		enum MemoryType {
			SRAM = 0,
			Flash,
			EEPROM
		};

		enum AllocationSearchDirection {
			TopDown = 0,
			BottomUp
		};

		uint8_t* allocate(const uint16_t numBytes, uint16_t* allocatedBytes, const AllocationSearchDirection direction);
		uint8_t* reallocate(const uint8_t* oldMemory, const uint16_t oldNumBytes, const uint16_t newNumBytes, uint16_t* allocatedBytes, const AllocationSearchDirection direction);
		void deallocate(const uint8_t* address, uint16_t numBytes);

		uint8_t read(const void* address, const MemoryType memType);
		bool write(const void* address, const uint8_t data, const MemoryType memType);

		uint16_t getTotalPages();
		uint16_t getTotalBytes();
		uint16_t getPageSize();

		bool isPageAvailable(const uint16_t pageNumber);
    }

}

#endif