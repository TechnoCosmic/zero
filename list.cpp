//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute	    Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#include <stdint.h>
#include "list.h"


using namespace zero;


template <class T>
List<T>::List() {
    _head = _tail = 0UL;
}


template <class T>
T* List<T>::getHead() {
    return _head;
}


template <class T>
T* List<T>::getTail() {
    return _tail;
}


template <class T>
void List<T>::prepend(T& item) {
    item._prev = 0UL;
    item._next = _head;

    if (_head) _head->_prev = &item;
    _head = &item;
    if (!_tail) _tail = &item;
}


template <class T>
void List<T>::append(T& item) {
    item._next = 0UL;
    item._prev = _tail;

    if (_tail) _tail->_next = &item;
    _tail = &item;
    if (!_head) _head = &item;
}


template <class T>
void List<T>::remove(T& item) {
    const bool wasHead = _head == &item;
    const bool wasTail = _tail == &item;

    T* p = item._prev;
    T* n = item._next;

    if (p) p->_next = n;
    if (n) n->_prev = p;

    if (wasHead) _head = n;
    if (wasTail) _tail = p;

    item._prev = 0UL;
    item._next = 0UL;
}


#include "list_classes.h"
