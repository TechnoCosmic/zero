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
#include "thread.h"

using namespace zero;

clicommand(ps, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    NamedObject::iterate(tx, [](void* data, NamedObject* obj) {
        Pipe* out = (Pipe*) data;

        if (obj->_objectType == ZeroObjectType::THREAD) {
            Thread* cur = (Thread*) obj;

            out->write(obj->_objectName, MemoryType::Flash);
            *out << "\r\n";
        }
    
        return true;
    });

    return 0;
});
