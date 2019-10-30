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
    const int rows = ROUND_UP(totalPages / PAGES_PER_ROW, 8);

    const uint16_t ttlRam = memory::getTotalBytes();
    uint16_t usedPages = 0UL;

    *tx << "Allocation table\r\n\n";

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < PAGES_PER_ROW; col++) {
            uint8_t pageNumber = row * PAGES_PER_ROW + col;

            if (pageNumber == totalPages) {
                *tx << "\r\n";
                goto exit;
            }
            
            if (memory::isPageAvailable(pageNumber)) {
                *tx << settextcolor(GREEN) << '-';

            } else {
                usedPages++;
                *tx << settextcolor(RED) << 'X';
            }
        }
        *tx << "\r\n";
    }

exit:

    const uint16_t usedBytes = memory::getPageSize() * usedPages;

    *tx << settextcolor(WHITE) << "\r\n";
    *tx << "Total allocatable SRAM: " << (int) ttlRam << "\r\n";
    *tx << "             Used SRAM:   " << (int) usedBytes << "\r\n";
    *tx << "        Available SRAM: " << (int) (ttlRam - usedBytes) << "\r\n";

    return 0;
});
