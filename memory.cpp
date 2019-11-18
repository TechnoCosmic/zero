/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "zero_config.h"
#include "memory.h"
#include "thread.h"
#include "atomic.h"
#include "string.h"
#include "util.h"

using namespace zero;
using namespace zero::memory;


// We use this to let the compiler reserve the SRAM for us.
// This helps avoid accidental memory corruption towards the
// lower addresses where globals are held.
uint8_t _memoryArea[DYNAMIC_BYTES] __attribute__((section(".heap")));


// The bit mapped page manager
PageManager<SRAM_PAGES> _sram;


// Returns the address for the start of a given page
uint16_t memory::getAddressForPage(const uint16_t pageNumber) {
    return ((uint16_t) _memoryArea) + (pageNumber * PAGE_BYTES);
}


// Return the page number for the given SRAM memory address
uint16_t memory::getPageForAddress(const uint16_t address) {

    return ((address) - ((uint16_t) _memoryArea)) / PAGE_BYTES;
}


// Returns the number of pages needed to store the supplied number of bytes
static constexpr uint16_t getNumPagesForBytes(const uint16_t bytes) {
    return ROUND_UP(bytes, PAGE_BYTES) / PAGE_BYTES;
}


// Allocates some memory. The amount of memory actually allocated is optionally
// returned in allocatedBytes, which will always be a multiple of the page size.
void* memory::allocate(const uint16_t numBytesRequested, uint16_t* allocatedBytes, const SearchStrategy strategy) {

    // critical section - one Thread allocating at a time, thank you
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

        const uint16_t numPages = getNumPagesForBytes(numBytesRequested);

        int16_t startPage = _sram.findFreePages(numPages, strategy);

        // make sure we've covered all the angles
        if (startPage == -1) {
            if (strategy == SearchStrategy::MiddleUp) {
                // MiddleUp failed, try MiddleDown before giving up entirely
                startPage = _sram.findFreePages(numPages, SearchStrategy::MiddleDown);

            } else {
                // MiddleDown failed, try MiddleUp before giving up entirely
                startPage = _sram.findFreePages(numPages, SearchStrategy::MiddleUp);
            }
        }

        // if there was a chunk the size we wanted
        if (startPage >= 0) {
            // mark the pages as no longer available
            for (uint16_t curPageNumber = startPage; curPageNumber < startPage + numPages; curPageNumber++) {
                _sram.markAsUsed(curPageNumber);
            }

            // tell the caller how much we gave them
            if (allocatedBytes) {
                *allocatedBytes = numPages * PAGE_BYTES;
            }

            // outta here
            return (uint8_t*) getAddressForPage(startPage);
        }

        // outta here
        return (uint8_t*) 0UL;
    }
}


// Frees up a chunk of previously allocated memory. In the interests of performance,
// there is no checking that the Thread 'owns' the memory being freed, nor is there
// a check to see if the memory was even allocated in the first place.
void memory::deallocate(const void* address, const uint16_t numBytes) {
    if (!numBytes) return;
    
	uint16_t numPages = getNumPagesForBytes(numBytes);
	uint16_t startPage = getPageForAddress((uint16_t) address);

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        // simply run from the first page to the last, ensuring the bit map says 'free'
        for (int curPage = startPage; curPage < startPage + numPages; curPage++) {
            _sram.markAsFree(curPage);
        }
    }
}


// Very unintelligent reallocator that simply finds a new chunk of the required size,
// copies the memory over to the new area, and deallocates the old chunk.
//
// More optimal approaches that could be taken include...
//
// - If shrinking the allocated memory size, we could just calculate the new number of
//   pages needed and just deallocate the difference from the end of the existing chunk
//
// - If expanding the allocated memory size, we could calculate the new number of pages
//   required and see if the difference is available at the end of the current chunk
//   before resorting to the full alloc/copy/dealloc sequence

void* memory::reallocate(   const void* oldMemory,       // the old memory previously allocated
                            const uint16_t oldNumBytes,     // the size of oldMemory
                            const uint16_t newNumBytes,     // new amount of memory needed
                            uint16_t* allocatedBytes,       // a pointer to how big the new memory is
                            const SearchStrategy strategy) {

    // critical section, no context switching
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        uint16_t allocated = 0UL;
        void* newMemory = 0UL;

        // nothing to do, clean up and exit
        if (newNumBytes == oldNumBytes) {
            allocated = newNumBytes;
            goto exit;
        }

        // try to allocate the new required RAM
        newMemory = memory::allocate(newNumBytes, &allocated, strategy);

        // the whole thing fails if we couldn't get the new memory
        if (!newMemory) {
            // outta here
            goto exit;
        }

        if (oldMemory) {
            // copy the old data to the new place
            memcpy((uint8_t*) newMemory, (uint8_t*) oldMemory, MIN(oldNumBytes, allocated));

            // free up the old memory
            memory::deallocate(oldMemory, oldNumBytes);
        }

exit:

        // tell the caller how much they have now
        if (allocatedBytes) {
            *allocatedBytes = allocated;
        }

        // outta here
        return newMemory;
    }
}


// Reads a byte from one of the three main on-board memory areas
uint8_t memory::read(const void* address, const MemoryType memType) {
	uint8_t rc = 0;

	switch (memType) {
		case MemoryType::SRAM:
			rc = *((uint8_t*) address);
		break;

		case MemoryType::FLASH:
			rc = pgm_read_byte(address);
		break;
		
		case MemoryType::EEPROM:
			rc = eeprom_read_byte((uint8_t*) address);
		break;
		
	}
	return rc;
}


// Writes a byte to one of the two main on-board writable memory areas
bool memory::write(const void* address, const uint8_t data, const MemoryType memType) {
	switch (memType) {
		case MemoryType::SRAM:
			*((uint8_t*) address) = data;
			return true;
		break;

		case MemoryType::FLASH:
			return false;
		break;

		case MemoryType::EEPROM:
			eeprom_write_byte((uint8_t*) address, data);
			return true;
		break;
	}
}


// Finds out if the supplied page is available for use
bool memory::isPageAvailable(const uint16_t pageNumber) {
	return _sram.isPageAvailable(pageNumber);
}


// Returns the total number of pages at the allocator's disposal
uint16_t memory::getTotalPages() {
	return _sram.getTotalPageCount();
}


// Returns the total number of bytes at the allocator's disposal
uint16_t memory::getTotalBytes() {
	return _sram.getTotalPageCount() * PAGE_BYTES;
}


// Returns the number of used pages
uint16_t memory::getUsedPages() {
	return _sram.getUsedPageCount();
}


// Returns the number of free pages
uint16_t memory::getUsedBytes() {
	return _sram.getUsedPageCount() * PAGE_BYTES;
}


// Returns the number of free pages
uint16_t memory::getFreeBytes() {
	return _sram.getFreePageCount() * PAGE_BYTES;
}


// Returns the page size, in bytes
uint16_t memory::getPageSizeBytes() {
    return PAGE_BYTES;
}
