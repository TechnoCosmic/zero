//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <string.h>
#include <avr/io.h>

#include "pagemanager.h"
#include "util.h"


using namespace zero;


// bit-field manipulation macros
#define BF_BYTE( b )        ( ( int )( b ) >> 3 )
#define BF_BIT( b )         ( ( int )( b ) & 0b111 )

#define BF_SET( bf,b )      bf[ BF_BYTE( b ) ] |= ( 1 << BF_BIT( b ) )
#define BF_CLR( bf,b )      bf[ BF_BYTE( b ) ] &= ~( 1 << BF_BIT( b ) )
#define BF_TST( bf,b )      ( bf[ BF_BYTE( b ) ] & ( 1 << BF_BIT( b ) ) )

// friendly names for the bit-field manipulators
#define IS_PAGE_AVAIL( b )  ( !BF_TST( _memoryMap, b ) )
#define MARK_AS_USED( b )   ( BF_SET( _memoryMap, b ) )
#define MARK_AS_FREE( b )   ( BF_CLR( _memoryMap, b ) )


/// @brief Determines if a given page is available for use
/// @param pageNumber The page number to check.
/// @returns ```true``` if the specified page is available, ```false``` otherwise.
/// @see markAsFree(), markAsUsed()
template <uint16_t PAGE_COUNT>
bool PageManager<PAGE_COUNT>::isPageAvailable( const uint16_t pageNumber ) const
{
    return IS_PAGE_AVAIL( pageNumber );
}


/// @brief Marks the given page as free
/// @param pageNumber The page number to mark as available.
/// @see markAsUsed(), isPageAvailable()
template <uint16_t PAGE_COUNT>
void PageManager<PAGE_COUNT>::markAsFree( const uint16_t pageNumber )
{
    MARK_AS_FREE( pageNumber );
}


/// @brief Marks the supplied page as used
/// @param pageNumber The page number to mark as unavailable.
/// @see markAsFree(), isPageAvailable()
template <uint16_t PAGE_COUNT>
void PageManager<PAGE_COUNT>::markAsUsed( const uint16_t pageNumber )
{
    MARK_AS_USED( pageNumber );
}


/// @brief Gets the total number of pages being managed
/// @returns The total number of pages being tracked by the PageManager.
/// @see getFreePageCount(), getUsedPageCount()
template <uint16_t PAGE_COUNT>
uint16_t PageManager<PAGE_COUNT>::getTotalPageCount() const
{
    return PAGE_COUNT;
}


/// @brief Gets the number of currently available pages
/// @returns The number of free pages.
/// @see getTotalPageCount(), getUsedPageCount()
template <uint16_t PAGE_COUNT>
uint16_t PageManager<PAGE_COUNT>::getFreePageCount() const
{
    uint16_t rc{ 0 };

    for ( uint16_t i = 0; i < getTotalPageCount(); i++ ) {
        if ( isPageAvailable( i ) ) {
            rc++;
        }
    }

    return rc;
}


/// @brief Gets the number of currently allocated pages
/// @returns The number of pages currently in use.
/// @see getFreePageCount(), getTotalPageCount()
template <uint16_t PAGE_COUNT>
uint16_t PageManager<PAGE_COUNT>::getUsedPageCount() const
{
    return getTotalPageCount() - getFreePageCount();
}


// search strategy function prototypes
static uint16_t getPageForSearchStep_TopDown( const uint16_t step, const uint16_t totalPages );
static uint16_t getPageForSearchStep_BottomUp( const uint16_t step, const uint16_t totalPages );


// search strategy - BottomUp starts searching the allocation table at the bottom and works up
static uint16_t getPageForSearchStep_BottomUp(
    const uint16_t step,
    const uint16_t )
{
    return step;
}


// search strategy - TopDown starts searching the allocation table at the top and works down
static uint16_t getPageForSearchStep_TopDown(
    const uint16_t step,
    const uint16_t totalPages )
{
    return totalPages - ( step + 1 );
}


namespace {
    // set up the vector table for the search strategies
    uint16_t ( *_strategies[] )( const uint16_t, const uint16_t ) = {
        getPageForSearchStep_TopDown,
        getPageForSearchStep_BottomUp,
    };
}


// This is the main workhorse for the memory allocator. Using only the search strategy supplied,
// find a supplied number of continguously available pages.
template <uint16_t PAGE_COUNT>
int16_t PageManager<PAGE_COUNT>::findFreePages(
    const uint16_t numPagesRequired,
    const memory::SearchStrategy strat ) const
{
    uint16_t startPage{ (uint16_t) -1 };
    uint16_t pageCount{ 0 };

    for ( uint16_t curStep = 0; curStep < PAGE_COUNT; curStep++ ) {
        const uint16_t curPage{ _strategies[ (int) strat ]( curStep, PAGE_COUNT ) };

        // If that page was free..
        if ( isPageAvailable( curPage ) ) {
            pageCount++;

            // if we didn't have a startPage, we do now
            if ( startPage == (uint16_t) -1 ) {
                startPage = curPage;
            }

            // if we've found the right number of pages, stop looking
            if ( pageCount == numPagesRequired ) {
                // MIN in case we searched backwards
                startPage = MIN( startPage, curPage );
                break;
            }
        }
        else {
            // wasn't free? start the search from scratch
            startPage = (uint16_t) -1;
            pageCount = 0;
        }
    }

    // if we couldn't find right number of pages
    if ( pageCount < numPagesRequired ) {
        return -1;
    }

    // if we get here, then we were successful at
    // finding what the caller wanted
    return startPage;
}


#include "pagemanager_classes.h"
