/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <stdint.h>
#include "textpipe.h"
#include "string.h"

using namespace zero;


// ctor
TextPipe::TextPipe(const char* name, uint16_t bufferSize, const bool strictSize) : Pipe(name, bufferSize, strictSize) {
    _base = 10;
    _width = -1;
    _fill = ' ';
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


// getBase
uint8_t TextPipe::getBase() {
    return _base;
}


// setWidth
void TextPipe::setWidth(const int16_t width) {
    _width = width;    
}

// getWidth
int16_t TextPipe::getWidth() {
    return _width;
}


// setFill
void TextPipe::setFill(const char c) {
    _fill = c;
}


// getFill
char TextPipe::getFill() {
    return _fill;
}


// setAlignment
void TextPipe::setAlignment(const Alignment alignment) {
    _alignment = alignment;    
}


// getAlignment
Alignment TextPipe::getAlignment() {
    return _alignment;
}


// Pushes a number of the filler character to the Pipe
static void pushFiller(TextPipe* p, const uint16_t count) {
    const char f = p->getFill();

    for (uint16_t i = 0; i < count; i++) {
        *p << f;
    }
}


// Pushes a supplied string into the Pipe, applying any
// alignment and padding as it does
static void pushString(const char* s, TextPipe* p) {
    int16_t width = p->getWidth();
    uint16_t frontPadding = 0;
    uint16_t backPadding = 0;

    if (width > 0) {
        int16_t excess = width - strlen(s);

        if (excess > 0) {
            switch (p->getAlignment()) {
                case Alignment::LEFT:
                    frontPadding = 0;
                    backPadding = excess;
                break;

                case Alignment::RIGHT:
                    frontPadding = excess;
                    backPadding = 0;
                break;

                case Alignment::CENTER:
                    frontPadding = excess / 2;
                    backPadding = excess - frontPadding;
                break;
            }
        }
        p->setWidth(-1);
    }

    pushFiller(p, frontPadding);
    *((Pipe*) p) << s;
    pushFiller(p, backPadding);
}


TextPipe& operator<<(TextPipe& out, const char c) {
    *((Pipe*) &out) << c;
	return out;
}


TextPipe& operator<<(TextPipe& out, const char* s) {
    pushString(s, &out);
	return out;
}

TextPipe& operator<<(TextPipe& out, const int v) {
	char buffer[32];
	int d = v;
	itoa(d, buffer, out.getBase(), true);
    pushString(buffer, &out);
	return out;
}
