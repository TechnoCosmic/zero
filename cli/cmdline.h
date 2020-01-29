//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#include <stdint.h>
#include "cli.h"


namespace zero {

    class CommandLine {
    public:

        CommandLine(const uint16_t size);
        ~CommandLine();

        bool registerKeyPress(const char c);
        void clear();
        void process(CliRx& rx, CliTx& tx);

        explicit operator bool() const;

    private:
        uint8_t tokenize(char* s, char* argv[]);

        char* _buffer;
        uint16_t _bufferSize;
        uint16_t _cursor;
    };

}
