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

Pipe& operator<<(Pipe& out, const settextcolor stc) {
    TextPipe* tp = (TextPipe*) &out;
    tp->setTextColor(stc._color);
    return out;
}


// back color
setbackcolor::setbackcolor(const Color color) : _color(color) { }

Pipe& operator<<(Pipe& out, const setbackcolor sbc) {
    TextPipe* tp = (TextPipe*) &out;
    tp->setBackColor(sbc._color);
    return out;
}


// base
setbase::setbase(const uint8_t base) : _base(base) { }

Pipe& operator<<(Pipe& out, const setbase sb) {
    TextPipe* tp = (TextPipe*) &out;
    tp->setBase(sb._base);
    return out;
}


// width
setw::setw(const int8_t width) : _width(width) { }

Pipe& operator<<(Pipe& out, const setw sw) {
    TextPipe* tp = (TextPipe*) &out;
    tp->setWidth(sw._width);
    return out;
}


// alignment
setalignment::setalignment(const Alignment alignment) : _alignment(alignment) { }

Pipe& operator<<(Pipe& out, const setalignment sa) {
    TextPipe* tp = (TextPipe*) &out;
    tp->setAlignment(sa._alignment);
    return out;
}
