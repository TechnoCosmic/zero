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
#include "thread.h"


using namespace zero;
using namespace zero::memory;


const PROGMEM char _unnamedDefault[] = "*** UNNAMED ***";

clicommand(ls, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    NamedObject::iterate(tx, [](void* data, NamedObject* obj) {
        TextPipe* out = (TextPipe*) data;

        if (obj->_objectName) {
            *out << PGM(obj->_objectName) << "\r\n";

        } else {
            *out << PGM(_unnamedDefault) << "\r\n";
        }

        return true;
    });

    return 0;
});


#endif // #ifdef CLICMD_LS
