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
#include "string.h"
#include "util.h"

using namespace zero;
using namespace zero::memory;


// So that the allocator has access to as
// much SRAM as possible on the target MCU
const uint16_t DYNAMIC_BYTES = (RAMEND - (256 + KERNEL_STACK_BYTES + GLOBALS_BYTES));

// Don't round this one up. If there's only enough
// RAM for a partial page, we can't use the page.
const uint16_t TOTAL_AVAILABLE_PAGES = DYNAMIC_BYTES / PAGE_BYTES;

// We use this to let the compiler reserve the SRAM for us.
// This helps avoid accidental memory corruption towards the
// lower addresses where globals are held.
uint8_t __attribute__((__aligned__(PAGE_BYTES))) _memoryArea[DYNAMIC_BYTES];

// round the pages to a multiple of 8 and then divide by 8
const uint16_t BYTES_FOR_BITMAP = ROUND_UP(TOTAL_AVAILABLE_PAGES, 8) / 8;

uint8_t _memoryMap[BYTES_FOR_BITMAP];

// bit-field manipulation macros
#define BF_BYTE(b) ((int)(b) >> 3)
#define BF_BIT(b) ((int)(b) & 0b111)

#define BF_SET(bf,b) bf[BF_BYTE(b)] |= (1 << BF_BIT(b))
#define BF_CLR(bf,b) bf[BF_BYTE(b)] &= ~(1 << BF_BIT(b))
#define BF_TST(bf,b) (bf[BF_BYTE(b)] & (1 << BF_BIT(b)))

// friendly names for the bit-field manipulators
#define IS_PAGE_AVAILABLE(b) (!BF_TST(_memoryMap,b))
#define MARK_AS_ALLOCATED(b) (BF_SET(_memoryMap,b))
#define MARK_AS_AVAILABLE(b) (BF_CLR(_memoryMap,b))

// Returns the address for the start of a given page
static constexpr uint16_t getAddressForPage(const uint16_t pageNumber) {
    return ((uint16_t) _memoryArea) + (pageNumber * PAGE_BYTES);
}

static constexpr uint16_t getPageForAddress(const uint16_t address) {
    return ((address) - ((uint16_t) _memoryArea)) / PAGE_BYTES;
}

static constexpr uint16_t getNumPagesForBytes(const uint16_t bytes) {
    return ROUND_UP(bytes, PAGE_BYTES) / PAGE_BYTES;
}

static int16_t findFreePages(const uint16_t numPagesRequired, const AllocationSearchDirection direction) {

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        int16_t firstPage, lastPage;
        int16_t adjust;

        // prepare the search loop according to the direction
        if (direction == AllocationSearchDirection::BottomUp) {
            firstPage = 0;
            lastPage = TOTAL_AVAILABLE_PAGES - 1;
            adjust = 1;

        } else {
            firstPage = TOTAL_AVAILABLE_PAGES - 1;
            lastPage = 0;
            adjust = -1;
        }

        // start looking for free pages
        int16_t startPage = -1;
        uint16_t pageCount = 0;

        for (int16_t curPageNumber = firstPage; curPageNumber >= 0 && curPageNumber <= TOTAL_AVAILABLE_PAGES-1; curPageNumber += adjust) {
            if (IS_PAGE_AVAILABLE(curPageNumber)) {
                // we have one more page than we had before
                pageCount++;

                // if we didn't have a startPage, we do now
                if (startPage == -1) {
                    startPage = curPageNumber;
                }

                // if we've found the right number of pages, stop looking
                if (pageCount == numPagesRequired) {
                    break;
                }

            } else {
                // wasn't free? start the search from scratch
                startPage = -1;
                pageCount = 0;
            }

        }

        // if we couldn't find the memory, return -1
        if (startPage == -1) {
            return -1;
        }

        // figure out what address to return to the caller
        if (direction == AllocationSearchDirection::BottomUp) {
            return startPage;                       // startPage is the lowest page
        } else {
            return startPage - pageCount + 1;       // startPage is the highest page
        }
    }
}

// Allocates some memory. The amount of memory actually allocated is optionally
// returned in allocatedBytes, which will always be a multiple of the page size.
uint8_t* memory::allocate(const uint16_t numBytesRequested, uint16_t* allocatedBytes, const AllocationSearchDirection direction) {

    // critical section - no context switching thanks
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
        const uint16_t numPages = getNumPagesForBytes(numBytesRequested);
        const int16_t startPage = findFreePages(numPages, direction);

        // if there was a chunk the size we wanted
        if (startPage >= 0) {
            // mark them as no longer available
            for (uint16_t curPageNumber = startPage; curPageNumber < startPage + numPages; curPageNumber++) {
                MARK_AS_ALLOCATED(curPageNumber);
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
void memory::deallocate(const uint8_t* address, const uint16_t numBytes) {
	uint16_t numPages = getNumPagesForBytes(numBytes);
	uint16_t startPage = getPageForAddress((uint16_t) address);

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        // simply run from the first page to the last, ensuring the bit map says 'free'
        for (int curPage = startPage; curPage < startPage + numPages; curPage++) {
            MARK_AS_AVAILABLE(curPage);
        }
    }
}

uint8_t* memory::reallocate(const uint8_t* oldMemory,       // the old memory previously allocated
                            const uint16_t oldNumBytes,     // the size of oldMemory
                            const uint16_t newNumBytes,     // new amount of memory needed
                            uint16_t* allocatedBytes,       // a pointer to how big the new memory is
                            const AllocationSearchDirection direction) {

    // critical section, no context switching
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        uint16_t allocated = 0UL;
        uint8_t* newMemory = 0UL;

        // nothing to do, clean up and exit
        if (newNumBytes == oldNumBytes) {
            goto exit;
        }

        // try to allocate the new required RAM
        newMemory = memory::allocate(newNumBytes, &allocated, direction);

        // the whole fails if we couldn't get the new memory
        if (!newMemory) {
            // outta here
            goto exit;
        }

        if (oldMemory) {
            // copy the old data to the new place
            memcpy(newMemory, oldMemory, MIN(oldNumBytes, allocated));

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

bool memory::isPageAvailable(const uint16_t pageNumber) {
	return IS_PAGE_AVAILABLE(pageNumber);
}

uint16_t memory::getTotalPages() {
	return TOTAL_AVAILABLE_PAGES;
}

uint16_t memory::getTotalBytes() {
	return TOTAL_AVAILABLE_PAGES * PAGE_BYTES;
}

uint16_t memory::getPageSize() {
    return PAGE_BYTES;
}