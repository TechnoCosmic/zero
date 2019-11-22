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

		// used for read() and write()
		enum MemoryType {
			SRAM = 0,											// Internal static RAM
			FLASH,												// Internal Flash stroage (PROGMEM)
			EEPROM,												// Internal EEPROM
		};

		// allocates a contiguous section of memory of a given size
		void* allocate(const uint16_t numBytes, uint16_t* allocatedBytes, const SearchStrategy strategy);

		// change the size of a previously allocated chunk of memory
		void* reallocate(const void* oldMemory, const uint16_t oldNumBytes, const uint16_t newNumBytes, uint16_t* allocatedBytes, const SearchStrategy strategy);

		// free up a previously allocated chunk of memory
		void deallocate(const void* address, uint16_t numBytes);

		// reads a byte from a given memory type
		uint8_t read(const void* address, const MemoryType memType);

		// writes a byte to a given memory type
		bool write(const void* address, const uint8_t data, const MemoryType memType);

		uint16_t getPageSizeBytes();							// returns of the size of a single page, in bytes

		uint16_t getTotalPages();								// returns the total number of pages available to the allocator
		uint16_t getTotalBytes();								// returns the total number of bytes under control of the allocator

		uint16_t getUsedPages();								// returns the number of allocated pages
		uint16_t getUsedBytes();								// returns the number of allocated bytes

		uint16_t getFreePages();								// return the number of free pages
		uint16_t getFreeBytes();								// return the number of free bytes

		bool isPageAvailable(const uint16_t pageNumber);		// determines if a given page is free
		uint16_t getAddressForPage(const uint16_t pageNumber);	// returns the start address of a given page
		uint16_t getPageForAddress(const uint16_t address);		// returns the page corresponding to an address

    }

}

#endif