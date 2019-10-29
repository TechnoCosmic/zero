/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 *
 * 	BLINK - Flashes two LEDs, connected to PB4/5
 * 
 */

#include <avr/io.h>
#include <util/delay.h>
#include "thread.h"

using namespace zero;

thread(flashPB4, 64, {
	DDRB = PORTB = (1 << PINB4);

	while (1) {
		PORTB ^= (1 << PINB4);
		_delay_ms(250);
	}

	return 0;
});


thread(flashPB5, 64, {
	DDRB = PORTB = (1 << PINB5);

	while (1) {
		PORTB ^= (1 << PINB5);
		_delay_ms(330);
	}

	return 0;
});
