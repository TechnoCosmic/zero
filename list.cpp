/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include "list.h"

using namespace zero;


template <class T>
bool List<T>::insertAfter(T* newItem, T* after) {
    const bool newTail = (after == getTail());

    newItem->_next = after->_next;
    newItem->_prev = after;

    after->_next = newItem;

    if (newTail) {
        _tail = newItem;
    }
}

template <class T>
bool List<T>::insertBefore(T* newItem, T* before) {
    const bool newHead = (before == getHead());

    newItem->_prev = before->_prev;
    newItem->_next = before;

    before->_prev = newItem;

    if (newHead) {
        _head = newItem;
    }
}

template <class T>
bool List<T>::append(T* item) {
    if (!canModify()) {
        return false;
    }

    item->_next = 0UL;
    item->_prev = _tail;

    if (_tail) {
        _tail->_next = item;
    }

    _tail = item;

    if (!_head) {
        _head = _tail;
    }

    return true;
}

template <class T>
bool List<T>::prepend(T* item) {
    if (!canModify()) {
        return false;
    }

    item->_next = _head;
    item->_prev = 0UL;

    if (_head) {
        _head->_prev = item;
    }

    _head = item;

    if (!_tail) {
        _tail = _head;
    }

    return true;
}

template <class T>
bool List<T>::remove(T* item) {
    const bool newHead = (item == getHead());
    const bool newTail = (item == getTail());

    if (!canModify()) {
        return false;
    }

    T* prev = item->_prev;
    T* next = item->_next;

    item->_prev = 0UL;
    item->_next = 0UL;

    if (prev) {
        prev->_next = next;
    }

    if (next) {
        next->_prev = prev;
    }

    if (newHead) {
        _head = next;
    }

    if (newTail) {
        _tail = prev;
    }

    return true;
}

template <class T>
bool List<T>::canModify() {
    return true;
}

template <class T>
T* List<T>::getHead() {
    return _head;
}

template <class T>
T* List<T>::getTail() {
    return _tail;
}

#include "list_classes.h"
