/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"


#ifdef CLICMD_THREAD_CTRL

#include "cli.h"
#include "textpipe.h"
#include "iomanip.h"
#include "thread.h"


#include <util/delay.h>

using namespace zero;


clicommand(pause, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

        for (int i = 1; i < argc; i++) {
            NamedObject* obj = NamedObject::find(argv[i]);

            if (obj && obj->_objectType == ZeroObjectType::THREAD) {
                Thread* t = (Thread*) obj;
                t->pause();
            }
        }
    }
    return 0;
});


clicommand(play, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {

        for (int i = 1; i < argc; i++) {
            NamedObject* obj = NamedObject::find(argv[i]);

            if (obj && obj->_objectType == ZeroObjectType::THREAD) {
                Thread* t = (Thread*) obj;
                t->run();
            }
        }
    }
    return 0;
});


#endif
