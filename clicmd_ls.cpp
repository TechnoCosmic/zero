/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"


#ifdef CLICMD_LS


#include <util/delay.h>
#include "cli.h"
#include "textpipe.h"
#include "iomanip.h"
#include "thread.h"


using namespace zero;


const PROGMEM char _unnamedDefault[] = "*** UNNAMED ***";

static Color _colorsForObjType[] {
    Color::GREEN,
    Color::BLUE,
    Color::YELLOW,
};

clicommand(ls, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    NamedObject::iterate(tx, [](void* data, uint16_t i, NamedObject* obj) {
        const char* nameToUse = (char*) TOT(obj->_objectName, _unnamedDefault);
        TextPipe* out = (TextPipe*) data;

        // set the right color
        *out << settextcolor(_colorsForObjType[obj->_objectType]);

        // no need to pad the last column, saves time
        if ((i & 0b11) != 3) {
            *out << setw(20) << setfill(' ');
        }

        *out << PGM(nameToUse);

        // if it's the last column, newline please
        if ((i & 0b11) == 3) {
            *out << endl;
        }

        return true;
    });

    *tx << white << endl;

    return 0;
});


#endif // #ifdef CLICMD_LS
