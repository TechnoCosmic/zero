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
#include "iomanip.h"

using namespace zero;


// ctor
TextPipe::TextPipe(const char* name, uint16_t bufferSize, const bool strictSize) : Pipe(name, bufferSize, strictSize) {
    _base = 10;
    _width = -1;
    _fill = ' ';
    _outputType = OutputType::VT100;
}


// setTextColor
void TextPipe::setTextColor(const Color color) {
    if (color != _textColor) {
        _textColor = color;

        if (_outputType == OutputType::VT100) {
            // push the escape codes to the terminal
            *this << "\e[3" << (char) ('0' + _textColor) << 'm';
        }
    }
}


// getTextColor
Color TextPipe::getTextColor() {
    return _textColor;
}


// setBackColor
void TextPipe::setBackColor(const Color color) {
    if (color != _backColor) {
        _backColor = color;

        if (_outputType == OutputType::VT100) {
            // push the escape codes to the terminal
            *this << "\e[4" << (char) ('0' + _backColor) << 'm';
        }
    }
}


// getBackColor
Color TextPipe::getBackColor() {
    return _backColor;
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


// setUppercase
void TextPipe::setUppercase(const bool v) {
    _uppercase = v;
}


// getUppercase
bool TextPipe::getUppercase() {
    return _uppercase;
}


// setOutputType
void TextPipe::setOutputType(const OutputType ot) {
    _outputType = ot;
}


// getOutputType
OutputType TextPipe::getOutputType() {
    return _outputType;
}


// Pushes a supplied string into the Pipe, applying any
// alignment and padding as it does
static void padString(const char* s, memory::MemoryType source, TextPipe* p) {
    int16_t width = p->getWidth();
    uint16_t leadingPadding = 0;
    uint16_t trailingPadding = 0;

    if (width > 0) {
        uint16_t len = 0;
        int16_t excess = 0;
        
        if (source == memory::MemoryType::SRAM) {
            len = strlen(s);
        } else {
            len = strlenpgm(s);
        }

        excess = width - len;

        if (excess > 0) {
            switch (p->getAlignment()) {
                case Alignment::LEFT:
                    leadingPadding = 0;
                    trailingPadding = excess;
                break;

                case Alignment::RIGHT:
                    leadingPadding = excess;
                    trailingPadding = 0;
                break;

                case Alignment::CENTER:
                    leadingPadding = excess / 2;
                    trailingPadding = excess - leadingPadding;
                break;
            }
        }

        // reset so that only this string
        // is subject to the padding
        p->setWidth(-1);
        p->setAlignment(Alignment::LEFT);
    }

    // push any leading padding
    *p << repeat(p->getFill(), leadingPadding);

    // the string itself
    p->write(s, source);

    // push any trailing padding
    *p << repeat(p->getFill(), trailingPadding);
}


TextPipe& operator<<(TextPipe& out, const char c) {
    *((Pipe*) &out) << c;
	return out;
}


TextPipe& operator<<(TextPipe& out, const char* s) {
    padString(s, memory::MemoryType::SRAM, &out);
	return out;
}

TextPipe& operator<<(TextPipe& out, const PGM s) {
    padString(s._s, memory::MemoryType::FLASH, &out);
	return out;
}


TextPipe& operator<<(TextPipe& out, const int16_t v) {
	char buffer[33];
	int16_t d = v;
	itoa(d, buffer, out.getBase(), false, out.getUppercase());
    padString(buffer, memory::MemoryType::SRAM, &out);
	return out;
}


TextPipe& operator<<(TextPipe& out, const uint16_t v) {
	char buffer[33];
	uint16_t d = v;
	itoa(d, buffer, out.getBase(), false, out.getUppercase());
    padString(buffer, memory::MemoryType::SRAM, &out);
	return out;
}


TextPipe& operator<<(TextPipe& out, const int32_t v) {
	char buffer[33];
	int32_t d = v;
	itoa(d, buffer, out.getBase(), false, out.getUppercase());
    padString(buffer, memory::MemoryType::SRAM, &out);
	return out;
}


TextPipe& operator<<(TextPipe& out, const uint32_t v) {
	char buffer[33];
	uint32_t d = v;
	itoa(d, buffer, out.getBase(), false, out.getUppercase());
    padString(buffer, memory::MemoryType::SRAM, &out);
	return out;
}
