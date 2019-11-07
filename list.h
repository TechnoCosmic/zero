/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_LIST_H
#define TCRI_ZERO_LIST_H

#include <stdint.h>

namespace zero {

    template <class T>
    class List {
    public:
        bool insertAfter(T* newItem, T* after);
        bool insertBefore(T* newItem, T* before);
        bool append(T* item);
        bool prepend(T* item);
        bool remove(T *item);
        bool canModify();

        T* getHead();
        T* getTail();

    private:
        T* _head;
        T* _tail;
    };

}

#endif
