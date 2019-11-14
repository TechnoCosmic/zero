/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"

#ifdef CLICMD_MEMDUMP

#include <stdint.h>
#include <avr/pgmspace.h>
#include "cli.h"
#include "textpipe.h"
#include "iomanip.h"
#include "string.h"
#include "memory.h"


using namespace zero;


const PROGMEM char ram_Usage[] = "Usage: ";
const PROGMEM char ram_Usage2[] = " <addr> (4 hex digits)";


static uint16_t getValue(const char c) {
	if (isdigit(c)) {
		return c - '0';
        
	} else {
		const char l = tolower(c);
        
		if (l >= 'a' && l <= 'f') {
			return (l - 'a') + 10;
		}
	}

	return 0;
}


static uint16_t hexStringToValue(const char* c) {
	uint16_t rc = 0;

	rc |= getValue(c[0]) << 12;
	rc |= getValue(c[1]) << 8;
	rc |= getValue(c[2]) << 4;
	rc |= getValue(c[3]);

	return rc;
}


static void setColorForByte(TextPipe* tx, uint8_t data) {
	// color coding
	if (data >= 32 && data <= 126) {
		if (isdigit(data)) {
			*tx << cyan;

		} else if (isalpha(data)) {
			*tx << yellow;

		} else {
			*tx << magenta;
		}
	} else {
		*tx << white;
	}
}


static void displayMemory(TextPipe* rx, TextPipe* tx, const uint16_t offset, memory::MemoryType source) {
	*tx << uppercase;
	
	*tx << setreverse(true);
	*tx << "     " << setfill('0');

	for (uint16_t i = 0; i < 16; i++) {
		*tx << setw(2) << hex << right << (int)((offset + i) & 0xF) << ' ';
		if (i == 7) {
			*tx << ' ';
		}
	}
	*tx << "  ";
	for (uint8_t i = 0; i < 16; i++) {
		*tx << setw(1) << hex << right << (int)((offset + i) & 0xF);
		if (i == 7) {
			*tx << ' ';
		}
	}
	*tx << setreverse(false);
	*tx << endl;
	for (uint16_t r = 0; r < 256; r += 16) {
		*tx << setreverse(true);
		*tx << setw(4) << hex << right << (uint32_t)(offset + r);
		*tx << setreverse(false);
		*tx << ' ';

		for (uint8_t c = 0; c < 16; c++) {
			uint8_t d = memory::read((void*) (r+c+offset), source);
			
			setColorForByte(tx, d);
			*tx << setw(2) << hex << right << (int) d << ' ';

			if (c == 7) {
				*tx << ' ';
			}
		}
		*tx << "  ";
		for (uint8_t c = 0; c < 16; c++) {
			uint8_t d = memory::read((void*) (r+c+offset), source);
			char o = '.';
			
			setColorForByte(tx, d);

			// if it's printable
			if (d >= 32 && d <= 126) {
				o = (char) d;
			}
			*tx << o;

			if (c == 7) {
				*tx << ' ';
			}
		}
		*tx << white << endl;
	}
	*tx << nouppercase << dec;
}


clicommand(rram, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
	uint16_t offset = 0;

	if (argc != 2 || strlen(argv[1]) != 4) {
		*tx << PGM(ram_Usage) << argv[0] << PGM(ram_Usage2) << endl;
		return 0;
	}

	offset = hexStringToValue(argv[1]);
	displayMemory(rx, tx, offset, memory::MemoryType::SRAM);

    return 0;
});


clicommand(rflash, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
	uint16_t offset = 0;

	if (argc != 2 || strlen(argv[1]) != 4) {
		*tx << PGM(ram_Usage) << argv[0] << PGM(ram_Usage2) << endl;
		return 0;
	}

	offset = hexStringToValue(argv[1]);
	displayMemory(rx, tx, offset, memory::MemoryType::FLASH);

    return 0;
});


clicommand(reeprom, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
	uint16_t offset = 0;

	if (argc != 2 || strlen(argv[1]) != 4) {
		*tx << PGM(ram_Usage) << argv[0] << PGM(ram_Usage2) << endl;
		return 0;
	}

	offset = hexStringToValue(argv[1]);
	displayMemory(rx, tx, offset, memory::MemoryType::EEPROM);

    return 0;
});


clicommand(wram, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
	uint16_t offset = 0;

	if (argc != 3 || strlen(argv[1]) != 4) {
		*tx << PGM(ram_Usage) << argv[0] << PGM(ram_Usage2) << endl;
		return 0;
	}

	offset = hexStringToValue(argv[1]);
    
	for (uint16_t i = 0; i < strlen(argv[2]); i++) {
		memory::write((uint8_t*) offset + i, argv[2][i], memory::MemoryType::SRAM);
	}

    return 0;
});


clicommand(weeprom, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
	uint16_t offset = 0;

	if (argc != 3 || strlen(argv[1]) != 4) {
		*tx << PGM(ram_Usage) << argv[0] << PGM(ram_Usage2) << endl;
		return 0;
	}

	offset = hexStringToValue(argv[1]);
    
	for (uint16_t i = 0; i < strlen(argv[2]); i++) {
		memory::write((uint8_t*) offset + i, argv[2][i], memory::MemoryType::EEPROM);
	}

    return 0;
});


#endif // #ifdef CLICMD_MEMDUMP
