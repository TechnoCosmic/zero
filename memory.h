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
#include "pagemanager.h"


namespace zero {

    namespace memory {

		enum MemoryType {
			SRAM = 0,
			FLASH,
			EEPROM,
		};

		void* allocate(const uint16_t numBytes, uint16_t* allocatedBytes, const SearchStrategy strategy);
		void* reallocate(const void* oldMemory, const uint16_t oldNumBytes, const uint16_t newNumBytes, uint16_t* allocatedBytes, const SearchStrategy strategy);
		void deallocate(const void* address, uint16_t numBytes);

		uint8_t read(const void* address, const MemoryType memType);
		bool write(const void* address, const uint8_t data, const MemoryType memType);

		uint16_t getPageSizeBytes();

		uint16_t getTotalPages();
		uint16_t getTotalBytes();

		uint16_t getUsedPages();
		uint16_t getUsedBytes();

		uint16_t getFreePages();
		uint16_t getFreeBytes();

		bool isPageAvailable(const uint16_t pageNumber);
		uint16_t getAddressForPage(const uint16_t pageNumber);
		uint16_t getPageForAddress(const uint16_t address);

    }

}

#endif