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
	*tx << "\r\n";
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
static const PROGMEM char STATE_WAITRD[] = "wait read";
static const PROGMEM char STATE_WAITWR[] = "wait write";


static const char* _stateString[] = {
	STATE_RUNNING,
	STATE_READY,
    STATE_PAUSED,
	STATE_TERMINATED,
    STATE_WAITTERM,
	STATE_WAITRD,
	STATE_WAITWR,
};


static const Color _stateColor[] = {
    Color::GREEN,
    Color::YELLOW,
    Color::YELLOW,
    Color::RED,
    Color::YELLOW,
    Color::CYAN,
    Color::MAGENTA,
};


#ifdef INSTRUMENTATION
    const PROGMEM char threadList_Header[] = "\e[7m   TID   NAME                   STATE         STCK RNGE    CUR  PEAK TOTAL     %CPU       TIME    \e[0m";
#else
    const PROGMEM char threadList_Header[] = "\e[7m   TID   NAME                   STATE         \e[0m";
#endif


void outputThread(Thread* t, TextPipe* tx) {
    *tx << setfill(' ');

    // Thread ID
    *tx << setw(6) << right << (int32_t) t->getThreadId() << "  ";

    // name
    *tx << setw(24) << PGM(t->_systemData._objectName);

    // state
    tx->setTextColor(_stateColor[t->_state]);
    *tx << setw(13) << (PGM) _stateString[t->_state] << white;

#ifdef INSTRUMENTATION

    uint16_t curStack = t->calcCurrentStackBytesUsed();
    uint16_t peakStack = t->calcPeakStackBytesUsed();
    uint32_t ttl = Thread::now();

    *tx << hex << uppercase;

    // stack start address
    *tx << ' ' << setw(4) << setfill('0') << right << (int32_t) t->getStackBottom();

    // stack end address
    *tx << '-' << setw(4) << setfill('0') << right << (int32_t) t->getStackTop();

    // stack usage
    *tx << " (" << dec << setfill(' ');

    // stack current
    *tx << setw(5) << right << (int32_t) curStack << '/';

    // stack peak
    *tx << setw(5) << right << (int32_t) peakStack << '/';

    // stack total available
    *tx << setw(5) << right << (int32_t) t->getStackSizeBytes() << ')';

    // CPU%
    int32_t pc = (t->_ticks * 1000UL) / ttl;
    *tx << "  " << setw(3) << right << (int32_t) (pc / 10) << '.' << pc % 10  <<  "%   ";

    // thread tick count
    displayTime(tx, t->_ticks);

    *tx << nouppercase;

#endif

    *tx << "\r\n";
}


clicommand(ps, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {

    *tx << PGM(threadList_Header) << "\r\n";

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
