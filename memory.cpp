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


// Don't round this one up. If there's only enough
// RAM for a partial page, we can't use the page.
const uint16_t TOTAL_AVAILABLE_PAGES = DYNAMIC_BYTES / PAGE_BYTES;

// We use this to let the compiler reserve the SRAM for us.
// This helps avoid accidental memory corruption towards the
// lower addresses where globals are held.
uint8_t __attribute__((__aligned__(256))) _memoryArea[DYNAMIC_BYTES];

// round the pages to a multiple of 8 and then divide by 8
const uint16_t BYTES_FOR_BITMAP = ROUND_UP(TOTAL_AVAILABLE_PAGES, 8) / 8;

// this is the SRAM page allocation table/memory map
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


static int16_t getPageForSearchStep_MiddleDown(const uint16_t step, const uint16_t totalPages);
static int16_t getPageForSearchStep_MiddleUp(const uint16_t step, const uint16_t totalPages);
static int16_t getPageForSearchStep_TopDown(const uint16_t step, const uint16_t totalPages);
static int16_t getPageForSearchStep_BottomUp(const uint16_t step, const uint16_t totalPages);


static int16_t getPageForSearchStep_BottomUp(const uint16_t step, const uint16_t totalPages) {
    return step;
}


static int16_t getPageForSearchStep_TopDown(const uint16_t step, const uint16_t totalPages) {
    return totalPages - (step + 1);
}


static int16_t getPageForSearchStep_MiddleDown(const uint16_t step, const uint16_t totalPages) {
    const int16_t midPoint = totalPages / 2;
    const int16_t rev = totalPages - (step + 1);
    const int16_t page = rev - midPoint;

    if (page < 0) {
        return -1;
    }
    return page;
}


static int16_t getPageForSearchStep_MiddleUp(const uint16_t step, const uint16_t totalPages) {
    const int16_t midPoint = totalPages / 2;
    const int16_t page = step + midPoint;

    if (page >= totalPages) {
        return -1;
    }
    return page;
}


static int16_t (*_strategies[])(const uint16_t, const uint16_t) = {
    getPageForSearchStep_TopDown,
    getPageForSearchStep_BottomUp,
    getPageForSearchStep_MiddleDown,
    getPageForSearchStep_MiddleUp,
};


// Returns the address for the start of a given page
uint16_t memory::getAddressForPage(const uint16_t pageNumber) {
    return ((uint16_t) _memoryArea) + (pageNumber * PAGE_BYTES);
}


// Return the page number for the given SRAM memory address
uint16_t memory::getPageForAddress(const uint16_t address) {
    return ((address) - ((uint16_t) _memoryArea)) / PAGE_BYTES;
}


static constexpr uint16_t getNumPagesForBytes(const uint16_t bytes) {
    return ROUND_UP(bytes, PAGE_BYTES) / PAGE_BYTES;
}


static int16_t findFreePages(const uint16_t numPagesRequired, const AllocationSearchDirection direction) {
    int16_t startPage = -1;
    uint16_t pageCount = 0;

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        for (uint16_t curStep = 0; curStep < TOTAL_AVAILABLE_PAGES; curStep++) {
            const uint16_t curPage =  _strategies[direction](curStep, TOTAL_AVAILABLE_PAGES);

            if (curPage == -1) {
                break;
            }

            if (IS_PAGE_AVAILABLE(curPage)) {
                // we have one more page than we had before
                pageCount++;

                // if we didn't have a startPage, we do now
                if (startPage == -1) {
                    startPage = curPage;
                }

                // if we've found the right number of pages, stop looking
                if (pageCount == numPagesRequired) {
                    startPage = MIN(startPage, curPage);
                    break;
                }

            } else {
                // wasn't free? start the search from scratch
                startPage = -1;
                pageCount = 0;
            }
        }
    }

    if (pageCount < numPagesRequired) {
        return -1;
    }

    return startPage;
}


// Allocates some memory. The amount of memory actually allocated is optionally
// returned in allocatedBytes, which will always be a multiple of the page size.
uint8_t* memory::allocate(const uint16_t numBytesRequested, uint16_t* allocatedBytes, const AllocationSearchDirection direction) {

    // critical section - no context switching thanks
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

        const uint16_t numPages = getNumPagesForBytes(numBytesRequested);
        int16_t startPage = findFreePages(numPages, direction);

        // make sure we've covered all the angles
        if (startPage == -1) {
            if (direction == AllocationSearchDirection::MiddleUp) {
                startPage = findFreePages(numPages, AllocationSearchDirection::MiddleDown);
            } else {
                startPage = findFreePages(numPages, AllocationSearchDirection::MiddleUp);
            }
        }

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
