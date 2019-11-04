/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"


#ifdef CLICMD_MEMMAP


#include "cli.h"
#include "memory.h"
#include "util.h"
#include "string.h"
#include "textpipe.h"
#include "iomanip.h"

using namespace zero;

const int PAGES_PER_ROW = 32;


static const PROGMEM char _totalAllocatableSram[] = "Total: ";
static const PROGMEM char _usedSram[] = " Used: ";
static const PROGMEM char _availableSram[] = "Avail: ";

static const char SYMBOL_ZEROPAGE = 'Z';
static const char SYMBOL_GLOBAL = 'G';
static const char SYMBOL_USED = '*';
static const char SYMBOL_AVAIL = '-';


static Color getColorForSymbol(const char s) {
    switch (s) {
        case SYMBOL_ZEROPAGE:
            return Color::WHITE;
            break;

        case SYMBOL_GLOBAL:
            return Color::YELLOW;
            break;

        case SYMBOL_USED:
            return Color::RED;
            break;

        case SYMBOL_AVAIL:
            return Color::GREEN;
            break;
    }
    return Color::WHITE;
}


static char getSymbolForAddress(const uint16_t address) {
    if (address >= 0 && address < 256) {
        return SYMBOL_ZEROPAGE;
    }

    if (address < memory::getAddressForPage(0)) {
        return SYMBOL_GLOBAL;
    }

    if (address < memory::getAddressForPage(memory::getTotalPages())) {
        const uint16_t curPage = memory::getPageForAddress(address);

        if (memory::isPageAvailable(curPage)) {
            return SYMBOL_AVAIL;
        } else {
            return SYMBOL_USED;
        }
    }

    return SYMBOL_GLOBAL;
}

clicommand(memmap, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    const int totalPages = memory::getTotalPages();
    const uint16_t ttlRam = memory::getTotalBytes();
    uint16_t usedPages = 0UL;


    for (uint16_t curAddress = 0; curAddress < RAMEND; curAddress += memory::getPageSize()) {
        const char symbol = getSymbolForAddress(curAddress);
        const Color col = getColorForSymbol(symbol);

        if (curAddress % (memory::getPageSize() * 32) == 0) {
            *tx << setreverse(true);
            *tx << white << right << uppercase << hex << setfill('0') << setw(4) << (int32_t) curAddress;
            *tx << setreverse(false);
        }

        if (curAddress % (memory::getPageSize() * 16) == 0) {
            *tx << ' ';
        }

        *tx << settextcolor(col) << (char) symbol;

        if ((curAddress + memory::getPageSize()) % (memory::getPageSize() * 32) == 0) {
            *tx << endl;
        }
    }
    
    *tx << left << dec << endl;

    // legend
    *tx << endl;
    *tx << settextcolor(getColorForSymbol(SYMBOL_ZEROPAGE)) << SYMBOL_ZEROPAGE << white << ": Zero page (I/O)\t";    
    *tx << settextcolor(getColorForSymbol(SYMBOL_GLOBAL)) << SYMBOL_GLOBAL << white << ": Globals" << endl;    
    *tx << settextcolor(getColorForSymbol(SYMBOL_USED)) << SYMBOL_USED << white << ": Allocated\t\t";    
    *tx << settextcolor(getColorForSymbol(SYMBOL_AVAIL)) << SYMBOL_AVAIL << white << ": Available" << endl;    

    return 0;
});


#endif // #ifdef CLICMD_MEMMAP
