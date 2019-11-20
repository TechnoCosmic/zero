/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <util/atomic.h>
#include "namedobject.h"
#include "string.h"
#include "thread.h"
#include "list.h"

using namespace zero;

static List<NamedObject> _systemObjectList;
static uint8_t _lockCount = 0;

// Adds a NamedObject to the _systemObjectsList
void NamedObject::add(NamedObject* obj) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		if (_lockCount == 0) {
			_systemObjectList.append(obj);
		}
	}
}

// removes the NamedObject from the _systemObjectsList
void NamedObject::remove(NamedObject* obj) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		if (_lockCount == 0) {
			_systemObjectList.remove(obj);
		}
	}
}

// locates a NamedObject by it's name
NamedObject* NamedObject::find(const char* name) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		NamedObject* rc = 0UL;
		NamedObject* cur = _systemObjectList.getHead();

		while (cur) {
			if(strcmppgm(cur->_objectName, (char*) name) == 0) {
				rc = cur;
				break;
			}
			cur = cur->_next;
		}

		return rc;
	}
}

// locates a NamedObject by it's name
NamedObject* NamedObject::findFirstByPattern(const char* pattern) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		NamedObject* rc = 0UL;
		NamedObject* cur = _systemObjectList.getHead();

		while (cur) {
			if(matches(cur->_objectName, (char*) pattern, true)) {
				rc = cur;
				break;
			}
			cur = cur->_next;
		}

		return rc;
	}
}

// locates a NamedObject by it's name and type
NamedObject* NamedObject::find(const char* name, const ZeroObjectType objType) {
	NamedObject* obj = NamedObject::find(name);

	if (obj && obj->_objectType == objType) {
		return obj;
	}

	return 0UL;
}

void NamedObject::iterate(void* data, bool (*func)(void* data, uint16_t i, NamedObject* obj)) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		_lockCount++;
	}

	NamedObject* cur = _systemObjectList.getHead();
	uint16_t i = 0;

	while (cur) {
		if (!func(data, i++, cur)) {
			break;
		}
		cur = cur->_next;
	}

	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		_lockCount--;
	}
}
