//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdint.h>
#include <string.h>

#include "memory.h"
#include "thread.h"
#include "util.h"


using namespace zero;
using namespace zero::memory;


namespace {
    // The SRAM that the dynamic allocator can hand out to callers
    uint8_t ALIGNED(64) _memoryArea[DYNAMIC_BYTES];

    // The bit mapped page manager
    PageManager<SRAM_PAGES> _sram;


    // Returns the address for the start of a given page
    constexpr uint16_t getAddressForPage(const uint16_t pageNumber)
    {
        return ((uint16_t) _memoryArea) + (pageNumber * PAGE_BYTES);
    }


    // Return the page number for the given SRAM memory address
    constexpr uint16_t getPageForAddress(const uint16_t address)
    {
        return ((address) - ((uint16_t) _memoryArea)) / PAGE_BYTES;
    }


    // Returns the number of pages needed to store the supplied number of bytes
    constexpr uint16_t getNumPagesForBytes(const uint16_t bytes)
    {
        return ROUND_UP(bytes, PAGE_BYTES) / PAGE_BYTES;
    }
}


// Allocates some memory. The amount of memory actually allocated is optionally
// returned in allocatedBytes, which will always be a multiple of the page size.
void* memory::allocate(
    const uint16_t numBytesRequested,
    uint16_t* allocatedBytes,
    const SearchStrategy strategy)
{
    void* rc = nullptr;

    if (allocatedBytes) {
        *allocatedBytes = 0UL;
    }

    // critical section - one Thread allocating at a time, thank you
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
        const uint16_t numPages = getNumPagesForBytes(numBytesRequested);
        const int16_t startPage = _sram.findFreePages(numPages, strategy);

        // if there was a chunk the size we wanted
        if (startPage >= 0) {
            // mark the pages as no longer available
            for (uint16_t curPageNumber = startPage;
                curPageNumber < startPage + numPages;
                curPageNumber++)
            {
                _sram.markAsUsed(curPageNumber);
            }

            // tell the caller how much we gave them
            if (allocatedBytes) {
                *allocatedBytes = numPages * PAGE_BYTES;
            }

            // outta here
            rc = (void*) getAddressForPage(startPage);
        }
    }

    return rc;
}


// Frees up a chunk of previously allocated memory. In the interests of
// performance, there is no checking that the Thread 'owns' the memory being
// freed, nor is there a check to see if the memory was even allocated in the
// first place.
void memory::free(const void* address, const uint16_t numBytes)
{
    if (address == nullptr) return;

    const uint16_t numPages = getNumPagesForBytes(numBytes);
    const uint16_t startPage = getPageForAddress((uint16_t) address);

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {        
        // run from the first page to the last, ensuring the bitmap says 'free'
        for (uint16_t curPage = startPage;
            curPage < startPage + numPages;
            curPage++)
        {
            _sram.markAsFree(curPage);
        }
    }
}


// overloads for new and delete
void* operator new(size_t size)
{
    return memory::allocate(size);
}


void operator delete(void* p, size_t size)
{
    memory::free(p, size);
}


// GCC housekeeping stuff
__extension__ typedef int __guard __attribute__((mode (__DI__)));


int __cxa_guard_acquire(__guard* g)
{
    return !*(char*)(g);
}


void __cxa_guard_release(__guard* g)
{
    *(char*) g = 1;
}


void __cxa_guard_abort(__guard* g)
{
    ;
}


void __cxa_pure_virtual(void)
{
    ;
}
