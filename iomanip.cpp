/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <stdint.h>
#include "iomanip.h"
#include "textpipe.h"

using namespace zero;


// text color
settextcolor::settextcolor(const Color color) : _color(color) { }

TextPipe& operator<<(TextPipe& out, const settextcolor stc) {
    out.setTextColor(stc._color);
    return out;
}


// back color
setbackcolor::setbackcolor(const Color color) : _color(color) { }

TextPipe& operator<<(TextPipe& out, const setbackcolor sbc) {
    out.setBackColor(sbc._color);
    return out;
}


// base
setbase::setbase(const uint8_t base) : _base(base) { }

TextPipe& operator<<(TextPipe& out, const setbase sb) {
    out.setBase(sb._base);
    return out;
}


// width
setw::setw(const int8_t width) : _width(width) { }

TextPipe& operator<<(TextPipe& out, const setw sw) {
    out.setWidth(sw._width);
    return out;
}


// fill
setfill::setfill(const char c) : _fill(c) { }

TextPipe& operator<<(TextPipe& out, const setfill sf) {
    out.setFill(sf._fill);
    return out;
}


// alignment
setalignment::setalignment(const Alignment alignment) : _alignment(alignment) { }

TextPipe& operator<<(TextPipe& out, const setalignment sa) {
    out.setAlignment(sa._alignment);
    return out;
}


// uppercase
setuppercase::setuppercase(const bool v) : _uppercase(v) { }

TextPipe& operator<<(TextPipe& out, const setuppercase su) {
    out.setUppercase(su._uppercase);
    return out;
}
