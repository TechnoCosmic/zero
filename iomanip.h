/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_IOMANIP_H
#define TCRI_ZERO_IOMANIP_H

#include <stdint.h>
#include "textpipe.h"

namespace zero {

    struct settextcolor {
        settextcolor(const Color color);
        Color _color;
    };

    struct setbackcolor {
        setbackcolor(const Color color);
        Color _color;
    };

    struct setbase {
        setbase(const uint8_t base);
        uint8_t _base;
    };

    struct setw {
        setw(const int8_t width);
        int8_t _width;
    };

    struct setalignment {
        setalignment(const Alignment alignment);
        Alignment _alignment;
    };

}

zero::TextPipe& operator<<(zero::TextPipe&, const zero::settextcolor);
zero::TextPipe& operator<<(zero::TextPipe&, const zero::setbackcolor);
zero::TextPipe& operator<<(zero::TextPipe&, const zero::setbase);
zero::TextPipe& operator<<(zero::TextPipe&, const zero::setw);
zero::TextPipe& operator<<(zero::TextPipe&, const zero::setalignment);

#endif