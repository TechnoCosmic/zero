/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_ATOMIC_H
#define TCRI_ZERO_ATOMIC_H


using namespace zero;


// Funky little ATOMIC_BLOCK macro clones for context switching
static __inline__ uint8_t __iForbidRetVal() {
	Thread::forbid();
	return 1;
}

static __inline__ void __iZeroRestore(const uint8_t* __tmr_save) {
	if (*__tmr_save) {
		Thread::permit();
	}
}

#define ZERO_ATOMIC_BLOCK(t) for ( t, __ToDo = __iForbidRetVal(); __ToDo ; __ToDo = 0 )
#define ZERO_ATOMIC_RESTORESTATE uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t)(Thread::isSwitchingEnabled())
#define ZERO_ATOMIC_FORCEON uint8_t tmr_save __attribute__((__cleanup__(__iZeroRestore))) = (uint8_t) 1)


#endif
