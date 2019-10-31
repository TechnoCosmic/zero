/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

/*
 *
 * TextPipe - A Pipe for formatting text, and for using VT100 terminals
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
        uint8_t getBase();
        
        void setWidth(const int16_t width);
        int16_t getWidth();
        
        void setFill(const char c);
        char getFill();

        void setAlignment(const Alignment alignment);
        Alignment getAlignment();

        void setUppercase(const bool v);
        bool getUppercase();

    private:
        Color _textColor;
        Color _backColor;
        uint8_t _base;
        int16_t _width;
        Alignment _alignment;
        char _fill;
        bool _uppercase;
    };

}

zero::TextPipe& operator<<(zero::TextPipe& out, const char c);
zero::TextPipe& operator<<(zero::TextPipe& out, const char* s);
zero::TextPipe& operator<<(zero::TextPipe& out, const zero::PGM s);
zero::TextPipe& operator<<(zero::TextPipe& out, const int16_t v);
zero::TextPipe& operator<<(zero::TextPipe& out, const uint16_t v);
zero::TextPipe& operator<<(zero::TextPipe& out, const int32_t v);
zero::TextPipe& operator<<(zero::TextPipe& out, const uint32_t v);

#endif