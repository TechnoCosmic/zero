/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <stdint.h>
#include "textpipe.h"

using namespace zero;


// ctor
TextPipe::TextPipe(const char* name, uint16_t bufferSize, const bool strictSize) : Pipe(name, bufferSize, strictSize) {
    _base = 10;
}


// setTextColor()
void TextPipe::setTextColor(const Color color) {
    if (color != _textColor) {
        _textColor = color;
        *this << "\e[3" << (char) ('0' + _textColor) << 'm';
    }
}


// setBackColor
void TextPipe::setBackColor(const Color color) {
    if (color != _backColor) {
        _backColor = color;
        *this << "\e[4" << (char) ('0' + _backColor) << 'm';
    }
}


// setBase
void TextPipe::setBase(const uint8_t base) {
    _base = base;
}


// setWidth
void TextPipe::setWidth(const int8_t width) {
    _width = width;    
}


// setAlignment
void TextPipe::setAlignment(const Alignment alignment) {
    _alignment = alignment;    
}
