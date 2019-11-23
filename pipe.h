/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_PIPE_H
#define TCRI_ZERO_PIPE_H


#include <stdint.h>
#include "namedobject.h"
#include "thread.h"
#include "memory.h"
#include "util.h"


namespace zero {

	class Pipe;
	
    typedef bool (*PipeFilter)(Pipe* p, uint8_t* data);

	class Pipe {
	public:
		static Pipe* find(const char* name);

		Pipe(const char* name, uint16_t bufferSize, const bool strictSize = false);
		~Pipe();

		bool isEmpty();
		bool isFull();

		bool read(uint8_t* data, const bool allowBlock);
		bool write(const uint8_t data, const bool allowBlock);
		bool write(const char* s);
		bool write(const char* s, memory::MemoryType memType);
		
        void setReadFilter(const PipeFilter newFilter);
        void setWriteFilter(const PipeFilter newFilter);

	private:
		uint16_t calcFirstFreeIndex();

		// This must be the first field in the class
		NamedObject _systemData;
		uint16_t _start;
		uint16_t _length;
		uint8_t* _buffer;
		uint16_t _bufferLength;
		uint16_t _allocatedBytes;

        PipeFilter _onRead;
        PipeFilter _onWrite;

		Thread* _currentReader;
		SignalMask _readSignals;

		Thread* _currentWriter;
		SignalMask _writeSignals;
	};

	struct PGM {
		PGM(const char* s);
		const char* _s;
	};

}

zero::Pipe& operator<<(zero::Pipe& out, const char c);
zero::Pipe& operator<<(zero::Pipe& out, const char* s);
zero::Pipe& operator<<(zero::Pipe& out, const int16_t v);
zero::Pipe& operator<<(zero::Pipe& out, const uint16_t v);
zero::Pipe& operator<<(zero::Pipe& out, const int32_t v);
zero::Pipe& operator<<(zero::Pipe& out, const uint32_t v);


#endif
