/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_NAMEDOBJECT_H
#define TCRI_ZERO_NAMEDOBJECT_H

#include <stdint.h>

namespace zero {

	enum ZeroObjectType {
		THREAD = 0,
		PIPE,
		SEMAPHORE,
		CLICOMMAND,
	};

	struct NamedObject {
		static void add(NamedObject*);
	
		static NamedObject* find(const char* name);
		static NamedObject* find(const char* name, const ZeroObjectType objType);
		static NamedObject* findByPattern(const char* pattern);
	
		static void remove(NamedObject*);
		static void iterate(void* data, bool (*func)(void* data, NamedObject* obj));

		ZeroObjectType _objectType;
		const char* _objectName;
		NamedObject* _prev;
		NamedObject* _next;
	};

}

#endif
