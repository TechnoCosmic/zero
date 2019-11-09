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
#include "pagemanager.h"
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
uint8_t __attribute__((__aligned__(256))) _memoryArea[DYNAMIC_BYTES];


// The bit mapped page manager
PageManager<zero::SRAM_PAGES> _sram;


// search strategy function prototypes
static int16_t getPageForSearchStep_MiddleDown(const uint16_t step, const uint16_t totalPages);
static int16_t getPageForSearchStep_MiddleUp(const uint16_t step, const uint16_t totalPages);
static int16_t getPageForSearchStep_TopDown(const uint16_t step, const uint16_t totalPages);
static int16_t getPageForSearchStep_BottomUp(const uint16_t step, const uint16_t totalPages);


// search strategy - BottomUp starts searching the allocation table at the bottom and works up
static int16_t getPageForSearchStep_BottomUp(const uint16_t step, const uint16_t totalPages) {
    return step;
}


// search strategy - TopDown starts searching the allocation table at the top and works down
static int16_t getPageForSearchStep_TopDown(const uint16_t step, const uint16_t totalPages) {
    return totalPages - (step + 1);
}


// search strategy - MiddleDown starts searching the allocation table at the midpoint and works down.
// NOTE: This does NOT wrap around or do anything weird like that - therefore IT ONLY SEARCHES HALF
// OF THE AVAILABLE SPACE. Use this in conjuction with other strategies if you don't want attempts to
// allocate fail when there is in fact some available in a different area.
static int16_t getPageForSearchStep_MiddleDown(const uint16_t step, const uint16_t totalPages) {
    const int16_t midPoint = totalPages / 2;
    const int16_t rev = totalPages - (step + 1);
    const int16_t page = rev - midPoint;

    if (page < 0) {
        return -1;
    }
    return page;
}


// search strategy - MiddleUp starts searching the allocation table at the midpoint and works up.
// NOTE: This does NOT wrap around or do anything weird like that - therefore IT ONLY SEARCHES HALF
// OF THE AVAILABLE SPACE. Use this in conjuction with other strategies if you don't want attempts to
// allocate fail when there is in fact some available in a different area.
static int16_t getPageForSearchStep_MiddleUp(const uint16_t step, const uint16_t totalPages) {
    const int16_t midPoint = totalPages / 2;
    const int16_t page = step + midPoint;

    if (page >= totalPages) {
        return -1;
    }
    return page;
}


// set up the vector table for the search strategies
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


// Returns the number of pages needed to store the supplied number of bytes
static constexpr uint16_t getNumPagesForBytes(const uint16_t bytes) {
    return ROUND_UP(bytes, PAGE_BYTES) / PAGE_BYTES;
}


// This is the main workhorse for the memory allocator. Using only the search strategy supplied,
// find a supplied number of continguously available pages.
static int16_t findFreePages(const uint16_t numPagesRequired, const SearchStrategy strat) {
    int16_t startPage = -1;
    uint16_t pageCount = 0;

    // critical section as we don't want many threads allocating at once
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        const uint16_t ttlPages = _sram.getPageCount();

        for (uint16_t curStep = 0; curStep < ttlPages; curStep++) {
            const uint16_t curPage =  _strategies[strat](curStep, ttlPages);

            // if the search strategy no longer
            // has any more pages in it's scope
            if (curPage == -1) {
                break;
            }

            // But if that page was free..
            if (_sram.isPageAvailable(curPage)) {
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

    // if we couldn't find right number of pages
    if (pageCount < numPagesRequired) {
        return -1;
    }

    // if we get here, then we were successful at
    // finding what the caller wanted
    return startPage;
}


// Allocates some memory. The amount of memory actually allocated is optionally
// returned in allocatedBytes, which will always be a multiple of the page size.
uint8_t* memory::allocate(const uint16_t numBytesRequested, uint16_t* allocatedBytes, const SearchStrategy direction) {

    // critical section - one Thread allocating at a time, thank you
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

        const uint16_t numPages = getNumPagesForBytes(numBytesRequested);
        int16_t startPage = findFreePages(numPages, direction);

        // make sure we've covered all the angles
        if (startPage == -1) {
            if (direction == SearchStrategy::MiddleUp) {
                // MiddleUp failed, try MiddleDown before giving up entirely
                startPage = findFreePages(numPages, SearchStrategy::MiddleDown);

            } else {
                // MiddleDown failed, try MiddleUp before giving up entirely
                startPage = findFreePages(numPages, SearchStrategy::MiddleUp);
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
void memory::deallocate(const uint8_t* address, const uint16_t numBytes) {
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

uint8_t* memory::reallocate(const uint8_t* oldMemory,       // the old memory previously allocated
                            const uint16_t oldNumBytes,     // the size of oldMemory
                            const uint16_t newNumBytes,     // new amount of memory needed
                            uint16_t* allocatedBytes,       // a pointer to how big the new memory is
                            const SearchStrategy direction) {

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

        // the whole thing fails if we couldn't get the new memory
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


// Finds out if the supplied page is available for use
bool memory::isPageAvailable(const uint16_t pageNumber) {
	return _sram.isPageAvailable(pageNumber);
}


// Returns the total number of pages at the allocator's disposal
uint16_t memory::getTotalPages() {
	return _sram.getPageCount();
}


// Returns the total number of bytes at the allocator's disposal
uint16_t memory::getTotalBytes() {
	return _sram.getPageCount() * PAGE_BYTES;
}


// Returns the page size, in bytes
uint16_t memory::getPageSizeBytes() {
    return PAGE_BYTES;
}
