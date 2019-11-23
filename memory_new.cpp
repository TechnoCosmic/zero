/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <stdint.h>
#include <stdlib.h>
#include "memory.h"
#include "pagemanager.h"

using namespace zero;


// overloads for new and delete
void* operator new(size_t size) {
    void* rc = memory::allocate(size, 0UL, memory::SearchStrategy::BottomUp);
    return rc;
}


void operator delete(void* p, size_t size) {
    memory::free((uint8_t*) p, size);
}


__extension__ typedef int __guard __attribute__((mode (__DI__)));


extern "C" int __cxa_guard_acquire(__guard*);
extern "C" void __cxa_guard_release(__guard*);
extern "C" void __cxa_guard_abort(__guard*);
extern "C" void __cxa_pure_virtual(void);


int __cxa_guard_acquire(__guard* g) {
    return !*(char*)(g);
}


void __cxa_guard_release(__guard* g) {
    *(char*) g = 1;
}


void __cxa_guard_abort(__guard* g) {
    ;
}


void __cxa_pure_virtual(void) {
    ;
}
