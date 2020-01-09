//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute		Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "thread.h"
#include "memory.h"
#include "debug.h"
#include "usart.h"
#include "suart.h"
#include "util.h"
#include "sram.h"


using namespace zero;


int idleThreadEntry()
{
    while (true);
}


void startup_sequence()
{
    ;
}
