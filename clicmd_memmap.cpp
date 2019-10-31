/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"
#include "cli.h"
#include "memory.h"
#include "util.h"
#include "string.h"
#include "textpipe.h"
#include "iomanip.h"

using namespace zero;

const int PAGES_PER_ROW = 32;


clicommand(memmap, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    const int totalPages = memory::getTotalPages();
    const uint16_t ttlRam = memory::getTotalBytes();
    uint16_t usedPages = 0UL;

    *tx << "Allocation table\r\n\n";

    for (uint16_t curPageNumber = 0; curPageNumber < totalPages; curPageNumber++) {
        if (memory::isPageAvailable(curPageNumber)) {
            *tx << green << '-';

        } else {
            usedPages++;
            *tx << red << '*';
        }

        if ((curPageNumber+1) % 32 == 0) {
            *tx << "\r\n";
        }
    }

    const uint16_t usedBytes = memory::getPageSize() * usedPages;

    *tx << white << dec << setfill(' ') << "\r\n\n";
    *tx << "Total allocatable SRAM: " << right << setw(5) << (int) ttlRam << "\r\n";
    *tx << "             Used SRAM: " << right << setw(5) << (int) usedBytes << "\r\n";
    *tx << "        Available SRAM: " << right << setw(5) << (int) (ttlRam - usedBytes) << "\r\n";
    *tx << left;

    return 0;
});
