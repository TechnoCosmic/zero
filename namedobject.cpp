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
static uint8_t _lockedCount = 0;

// Adds a NamedObject to the _systemObjectsList
void NamedObject::add(NamedObject* obj) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		if (_lockedCount == 0) {
			_systemObjectList.append(obj);
		}
	}
}

// removes the NamedObject from the _systemObjectsList
void NamedObject::remove(NamedObject* obj) {
	ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
		
		if (_lockedCount == 0) {
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

// locates a NamedObject by it's name and type
NamedObject* NamedObject::find(const char* name, const ZeroObjectType objType) {
	NamedObject* obj = NamedObject::find(name);

	if (obj && obj->_objectType == objType) {
		return obj;
	}
	return 0UL;
}

void NamedObject::iterate(void* data, bool (*func)(void* data, NamedObject* obj)) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		_lockedCount++;
	}
	NamedObject* cur = _systemObjectList.getHead();

	while (cur) {
		if (!func(data, cur)) {
			break;
		}
		cur = cur->_next;
	}
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		_lockedCount--;
	}
}
