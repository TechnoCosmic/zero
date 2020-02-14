//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdint.h>
#include "list.h"


using namespace zero;


template <class T>
List<T>::List()
{
    _head = _tail = nullptr;
}


template <class T>
T* List<T>::getHead() const
{
    return _head;
}


template <class T>
T* List<T>::getTail() const
{
    return _tail;
}


template <class T>
void List<T>::prepend( T& item )
{
    item._prev = nullptr;
    item._next = _head;

    if ( _head ) {
        _head->_prev = &item;
    }

    _head = &item;

    if ( !_tail ) {
        _tail = &item;
    }
}


template <class T>
void List<T>::append( T& item )
{
    item._next = nullptr;
    item._prev = _tail;

    if ( _tail ) {
        _tail->_next = &item;
    }

    _tail = &item;

    if ( !_head ) {
        _head = &item;
    }
}


template <class T>
void List<T>::remove( T& item )
{
    const bool wasHead = _head == &item;
    const bool wasTail = _tail == &item;

    T* p = item._prev;
    T* n = item._next;

    if ( p ) {
        p->_next = n;
    }

    if ( n ) {
        n->_prev = p;
    }

    if ( wasHead ) {
        _head = n;
    }

    if ( wasTail ) {
        _tail = p;
    }

    item._prev = nullptr;
    item._next = nullptr;
}


template <class T>
void List<T>::insertBefore( T& item, T& before )
{
    const bool newHead = &before == _head;

    item._next = &before;
    item._prev = before._prev;

    before._prev = &item;

    if ( item._prev ) {
        item._prev->_next = &item;
    }

    if ( newHead ) {
        _head = &item;
    }
}


template <class T>
void OffsetList<T>::remove( T& item )
{
    // we have to adjust the next item's offset before
    // being allowed to the remove the item from the list
    if ( T* n = item._next ) {
        n->_timeoutOffset += item._timeoutOffset;
    }

    List<T>::remove( item );
}


template <class T>
void OffsetList<T>::insertByOffset( T& item, const uint32_t intendedOffsetFromNow )
{
    bool added = false;
    uint32_t curOffsetFromNow = 0ULL;
    T* cur = List<T>::getHead();

    while ( cur ) {
        curOffsetFromNow += cur->_timeoutOffset;

        if ( curOffsetFromNow > intendedOffsetFromNow ) {
            // insert before cur
            List<T>::insertBefore( item, *cur );

            // adjust the delta of the incoming item
            item._timeoutOffset = intendedOffsetFromNow - ( curOffsetFromNow - cur->_timeoutOffset );

            // delta of item after us is reduced by our delta
            cur->_timeoutOffset -= item._timeoutOffset;

            // mark it as done so no append occurs later
            added = true;

            // escape
            break;
        }

        cur = cur->_next;
    }

    // if the item hasn't found a home yet,
    // simply throw it on the end of the list
    if ( !added ) {
        List<T>::append( item );

        // adjust the delta of the incoming item
        item._timeoutOffset = intendedOffsetFromNow - curOffsetFromNow;
    }
}


#include "list_classes.h"
