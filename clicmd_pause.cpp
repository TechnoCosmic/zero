/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"


#ifdef CLICMD_THREAD_CTRL

#include <util/delay.h>
#include "cli.h"
#include "textpipe.h"
#include "iomanip.h"
#include "thread.h"
#include "atomic.h"

using namespace zero;


static const PROGMEM char _paused[] = "Paused ";
static const PROGMEM char _resumed[] = "Resumed ";
static const PROGMEM char _threads[] = " threads";

clicommand(pause, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    int c = 0;

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

        for (int i = 1; i < argc; i++) {
            NamedObject* obj = NamedObject::find(argv[i]);

            if (obj && obj->_objectType == ZeroObjectType::THREAD) {
                Thread* t = (Thread*) obj;
                
                if (t->pause()) {
                    c++;
                }
            }
        }
    }

    *tx << PGM(_paused) << c << PGM(_threads) << endl;

    return 0;
});


clicommand(play, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    int c = 0;

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

        for (int i = 1; i < argc; i++) {
            NamedObject* obj = NamedObject::find(argv[i]);

            if (obj && obj->_objectType == ZeroObjectType::THREAD) {
                Thread* t = (Thread*) obj;

                if (t->run()) {
                    c++;
                }
            }
        }
    }

    *tx << PGM(_resumed) << c << PGM(_threads) << endl;

    return 0;
});


#endif
