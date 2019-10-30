/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_TEXTPIPE_H
#define TCRI_ZERO_TEXTPIPE_H

#include <stdint.h>
#include "pipe.h"

namespace zero {

    enum Alignment {
        LEFT = 0,
        CENTER,
        RIGHT,
    };

    enum Color {
        BLACK = 0,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE,
    };

    class TextPipe : public Pipe {
    public:
		TextPipe(const char* name, uint16_t bufferSize, const bool strictSize = false);

        void setTextColor(const Color color);
        void setBackColor(const Color color);
        void setBase(const uint8_t base);
        void setWidth(const int8_t width);
        void setAlignment(const Alignment alignment);

    private:
        Color _textColor;
        Color _backColor;
        uint8_t _base;
        int8_t _width;
        Alignment _alignment;
    };

}

zero::TextPipe& operator<<(zero::TextPipe& out, const char* s);
zero::TextPipe& operator<<(zero::TextPipe& out, const char c);
zero::TextPipe& operator<<(zero::TextPipe& out, const int v);

#endif