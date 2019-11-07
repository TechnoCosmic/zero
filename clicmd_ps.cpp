/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "zero_config.h"


#ifdef CLICMD_PS


#include "cli.h"
#include "textpipe.h"
#include "iomanip.h"
#include "thread.h"


using namespace zero;
using namespace zero::memory;


static const uint32_t STACK_WARN_PERCENTAGE = 80UL;
static const uint32_t STACK_SCREAM_PERCENTAGE = 100UL;


// format a millisecond input as hr:mn:ss.mmm
// directly into a supplied Pipe
static void displayTime(TextPipe* tx, uint32_t ms) {
	*tx << setfill('0') << dec;

	// hours
	*tx << setw(2) << right << (int) (ms / 3600000L) << ':';

	// minutes
	*tx << setw(2) << right << (int) ((ms / 60000) % 60) << ':';

	// seconds
	*tx << setw(2) << right << (int) ((ms / 1000) % 60) << '.';

	// milliseconds
	*tx << setw(3) << right << (int) (ms % 1000);
}


const PROGMEM char uptime_Header[] = "Uptime: ";


void displayUptime(TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
	*tx << PGM(uptime_Header);
	displayTime(tx, Thread::now());
	*tx << endl;
}


clicommand(uptime, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
	displayUptime(rx, tx, argc, argv);
	return 0;
});


static const PROGMEM char STATE_RUNNING[] = "running";
static const PROGMEM char STATE_READY[] = "ready";
static const PROGMEM char STATE_PAUSED[] = "paused";
static const PROGMEM char STATE_TERMINATED[] = "terminated";
static const PROGMEM char STATE_WAITTERM[] = "wait term";
static const PROGMEM char STATE_WAITATOMICWR[] = "wait lock";
static const PROGMEM char STATE_WAITRD[] = "wait read";
static const PROGMEM char STATE_WAITWR[] = "wait write";


static const char* _stateString[] = {
	STATE_RUNNING,
	STATE_READY,
    STATE_PAUSED,
	STATE_TERMINATED,
    STATE_WAITTERM,
    STATE_WAITATOMICWR,
	STATE_WAITRD,
	STATE_WAITWR,
};


static const Color _stateColor[] = {
    Color::GREEN,
    Color::WHITE,
    Color::YELLOW,
    Color::RED,
    Color::YELLOW,
    Color::YELLOW,
    Color::CYAN,
    Color::MAGENTA,
};


#ifdef INSTRUMENTATION
    const PROGMEM char threadList_Header[] = "   TID  NAME                    STATE         STCK RNGE    CUR  PEAK TOTAL       TIME    ";
#else
    const PROGMEM char threadList_Header[] = "   TID  NAME                    STATE         STCK RNGE    CUR TOTAL ";
#endif


// Sends a single Thread's details to the TextPipe
void outputThread(Thread* t, TextPipe* tx) {
    const uint16_t stackWarnLevel = (STACK_WARN_PERCENTAGE * t->getStackSizeBytes()) / 100UL;
    const uint16_t stackScreamLevel = (STACK_SCREAM_PERCENTAGE * t->getStackSizeBytes()) / 100UL;


    *tx << setfill(' ');

    // Thread ID
    *tx << setw(6) << right << (int32_t) t->getThreadId() << "  ";

    // name
    *tx << setw(24) << PGM(t->_systemData._objectName);

    // state
    tx->setTextColor(_stateColor[t->_state]);
    *tx << setw(13) << (PGM) _stateString[t->_state] << white;

    // stack stuff
    *tx << hex << uppercase;

    // stack start address
    *tx << ' ' << setw(4) << setfill('0') << right << (int32_t) t->getStackBottom();

    // stack end address
    *tx << '-' << setw(4) << setfill('0') << right << (int32_t) t->getStackTop();

    // stack usage
    *tx << " (" << dec << setfill(' ');

    // stack current
    Color curStackColor = Color::WHITE;
    uint16_t curStack = t->calcCurrentStackBytesUsed();

    if (curStack >= stackScreamLevel) {
        curStackColor =  Color::RED;

    } else if (curStack >= stackWarnLevel) {
        curStackColor = Color::YELLOW;
    }

    *tx << settextcolor(curStackColor) << setw(5) << right << (int32_t) curStack << white << '/';

#ifdef INSTRUMENTATION

    Color peakStackColor = Color::WHITE;
    uint16_t peakStack = t->calcPeakStackBytesUsed();
    uint32_t ttl = Thread::now();

    if (peakStack >= stackScreamLevel) {
        peakStackColor =  Color::RED;

    } else if (peakStack >= stackWarnLevel) {
        peakStackColor = Color::YELLOW;
    }

    // stack peak
    *tx << settextcolor(peakStackColor) << setw(5) << right << (int32_t) peakStack << white << '/';

#endif

    // stack total available
    *tx << setw(5) << right << (int32_t) t->getStackSizeBytes() << ')';

#ifdef INSTRUMENTATION

    *tx << "  ";

    // thread tick count
    displayTime(tx, t->_ticks);

#endif

    *tx << nouppercase;
    *tx << endl;
}


clicommand(ps, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    *tx << setreverse(true);
    *tx << PGM(threadList_Header);
    *tx << setreverse(false);
    *tx << endl;

    NamedObject::iterate(tx, [](void* data, NamedObject* obj) {

        if (obj->_objectType == ZeroObjectType::THREAD) {
            outputThread((Thread*) obj, (TextPipe*) data);
        }

        return true;
    });

	displayUptime(rx, tx, argc, argv);

    return 0;
});


#endif // #ifdef CLICMD_PS
