/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <util/atomic.h>
#include <avr/pgmspace.h>
#include "pipe.h"
#include "memory.h"
#include "thread.h"
#include "string.h"

using namespace zero;

// find a Pipe by it's name
static Pipe* find(const char* name) {
	return (Pipe*) NamedObject::find(name, ZeroObjectType::PIPE);
}

// ctor
Pipe::Pipe(const char* name, const uint16_t bufferSize, const bool strictSize) {
	uint16_t allocated = 0UL;

	_buffer = memory::allocate(bufferSize, &allocated, memory::AllocationSearchDirection::BottomUp);

	if (strictSize) {
		_bufferLength = bufferSize;
	} else {
		_bufferLength = allocated;
	}

	_start = _length = 0;

	_systemData._objectName = name;
	_systemData._objectType = PIPE;

	NamedObject::add((NamedObject*) this);
}

// dtor
Pipe::~Pipe() {
	NamedObject::remove((NamedObject*) this);
	memory::deallocate(_buffer, _bufferLength);
	_buffer = 0UL;
}

// Returns true if the Pipe contains no data, false otherwise
bool Pipe::isEmpty() {
	return _length == 0;
}

// Returns true if the Pipe is full, false otherwise
bool Pipe::isFull() {
	return _length == _bufferLength;
}

// Calculates the index of the first free spot in the Pipe
uint16_t Pipe::calcFirstFreeIndex() {
	uint16_t rc = _start + _length;

	if (rc >= _bufferLength) {
		rc -= _bufferLength;
	}
	return rc;
}

// Writes a byte of data to the Pipe. If the Pipe is full and allowBlock
// is true, the calling Thread will block until room becomes available
// in the Pipe. Return true if the data was successfully written to the
// Pipe, false otherwise.
bool Pipe::write(const uint8_t data, const bool allowBlock) {
	if (!allowBlock && isFull()) {
		return false;
	}
	while (allowBlock && isFull()) {
		Thread::block(ThreadState::WAIT_WRITE, (uint32_t) this);
	}
	// NOTE: There is potentially a race condition here if multiple
	// threads are trying to write to the same full Pipe. So don't
	// do that. If you need to multiplex, work out a proper method
	// of doing it yourself.

	// NOTE: This should remain as traditional ATOMIC_BLOCK instead of
	// ZERO_ATOMIC_BLOCK as Pipes may be used from ISRs - eg USART

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// if there's a filter
		uint8_t dataToWrite = data;
		bool doIt = true;

		if (_onWrite) {
			doIt = _onWrite(&dataToWrite);
		}
		if (doIt) {
			_buffer[calcFirstFreeIndex()] = dataToWrite;
			_length++;
			// unblock any Thread that was blocked
			// waiting for data to become available.
			Thread::unblock(ThreadState::WAIT_READ, (uint32_t) this);
		}
	}
	return true;
}

// Outputs an SRAM-based string to the Pipe. Returns true if the write
// was successful, false otherwise.
bool Pipe::write(const char* s) {
	return write(s, memory::MemoryType::SRAM);
}

// Outputs a string to the Pipe. Returns true if the write
// was successful, false otherwise.
bool Pipe::write(const char* s, memory::MemoryType source) {
	char a = memory::read(s, source);
	while (a) {
		write(a, true);
		s++;
		a = memory::read(s, source);
	}
	return true;
}

// Reads a byte of data from the Pipe. 
bool Pipe::read(uint8_t* data, const bool allowBlock) {
	if (!allowBlock && isEmpty()) {
		return false;
	}
	while (allowBlock && isEmpty()) {
		Thread::block(ThreadState::WAIT_READ, (uint32_t) this);
	}
	// NOTE: There is potentially a race condition here if multiple
	// threads are trying to read from the same empty Pipe. To fix this,
	// perhaps I need to consider a proper queue for multiple reader/
	// writer Threads, though I am wondering at the logic behind
	// multiple Threads reading from the same Pipe, in a non-deterministic
	// order. A queue wouldn't solve this, unless the order within the
	// queue could change based on.. protocol? That's beyond the scope of
	// a simple Pipe class to solve.
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		bool doIt = true;

		*data = _buffer[_start];

		if (_onRead) {
			doIt = _onRead(data);
		}
		if (doIt) {
			_start++;
			_length--;

			if (_start == _bufferLength) {
				_start = 0;
			}

			Thread::unblock(ThreadState::WAIT_WRITE, (uint32_t) this);
			return true;
		}
	}
	return false;
}

void Pipe::setReadFilter(const PipeFilter newFilter) {
    _onRead = newFilter;
}

void Pipe::setWriteFilter(const PipeFilter newFilter) {
    _onWrite = newFilter;
}


PGM::PGM(const char* s) : _s(s) { }


Pipe& operator<<(Pipe& out, const char c) {
	out.write(c, true);
	return out;
}


Pipe& operator<<(Pipe& out, const char* s) {
	out.write(s);
	return out;
}


Pipe& operator<<(Pipe& out, const PGM s) {
	out.write(s._s, memory::MemoryType::Flash);
	return out;
}


Pipe& operator<<(Pipe& out, const int16_t v) {
	char buffer[33];
	int16_t d = v;
	out.write(itoa(d, buffer, 10, false));
	return out;
}


Pipe& operator<<(Pipe& out, const uint16_t v) {
	char buffer[33];
	uint16_t d = v;
	out.write(itoa(d, buffer, 10, false));
	return out;
}


Pipe& operator<<(Pipe& out, const int32_t v) {
	char buffer[33];
	int32_t d = v;
	out.write(itoa(d, buffer, 10, false));
	return out;
}


Pipe& operator<<(Pipe& out, const uint32_t v) {
	char buffer[33];
	uint32_t d = v;
	out.write(itoa(d, buffer, 10, false));
	return out;
}
