/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"
#include "pagemanager.h"

using namespace zero;


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


// Has the page been marked as used, or is it free?
template <uint16_t PAGE_COUNT>
bool PageManager<PAGE_COUNT>::isPageAvailable(const uint16_t pageNumber) {
    return IS_PAGE_AVAILABLE(pageNumber);
}


// Mark the supplied page as free
template <uint16_t PAGE_COUNT>
void PageManager<PAGE_COUNT>::markAsFree(const uint16_t pageNumber) {
    MARK_AS_AVAILABLE(pageNumber);
}


// Mark the supplied page as used
template <uint16_t PAGE_COUNT>
void PageManager<PAGE_COUNT>::markAsUsed(const uint16_t pageNumber) {
    MARK_AS_ALLOCATED(pageNumber);
}


// Returns the number of pages being managed
template <uint16_t PAGE_COUNT>
uint16_t PageManager<PAGE_COUNT>::getPageCount() {
    return PAGE_COUNT;
}


#include "pagemanager_classes.h"
