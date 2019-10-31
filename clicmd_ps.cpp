/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"
#include "cli.h"
#include "textpipe.h"
#include "iomanip.h"
#include "thread.h"

using namespace zero;
using namespace zero::memory;


clicommand(ps, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    NamedObject::iterate(tx, [](void* data, NamedObject* obj) {
        TextPipe* out = (TextPipe*) data;

        if (obj->_objectType == ZeroObjectType::THREAD) {
            Thread* cur = (Thread*) obj;

            *out << setfill(' ') << setw(24) << PGM(obj->_objectName);
#ifdef INSTRUMENTATION
            *out << setfill(' ') << setw(6) << right << dec << (uint16_t) (cur->_ticks);
#endif
            *out << "\r\n";
        }
    
        return true;
    });

    return 0;
});
