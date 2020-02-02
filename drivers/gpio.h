//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_GPIO


#ifndef TCRI_ZERO_GPIO_H
#define TCRI_ZERO_GPIO_H


#include <stdint.h>
#include <avr/io.h>


namespace zero {


    typedef uint32_t PinField;


    // Maps a port number and pin number to a simple integer
    constexpr uint32_t pinPort(const uint8_t port, const uint8_t pin)
    {
        return ((uint32_t) port << 3) | ((uint32_t) pin & 7);
    }


    #ifdef PORTA
        const PinField ZERO_PINA0 = (1L << pinPort(0, 0));
        const PinField ZERO_PINA1 = (1L << pinPort(0, 1));
        const PinField ZERO_PINA2 = (1L << pinPort(0, 2));
        const PinField ZERO_PINA3 = (1L << pinPort(0, 3));
        const PinField ZERO_PINA4 = (1L << pinPort(0, 4));
        const PinField ZERO_PINA5 = (1L << pinPort(0, 5));
        const PinField ZERO_PINA6 = (1L << pinPort(0, 6));
        const PinField ZERO_PINA7 = (1L << pinPort(0, 7));
    #endif

    #ifdef PORTB
        const PinField ZERO_PINB0 = (1L << pinPort(1, 0));
        const PinField ZERO_PINB1 = (1L << pinPort(1, 1));
        const PinField ZERO_PINB2 = (1L << pinPort(1, 2));
        const PinField ZERO_PINB3 = (1L << pinPort(1, 3));
        const PinField ZERO_PINB4 = (1L << pinPort(1, 4));
        const PinField ZERO_PINB5 = (1L << pinPort(1, 5));
        const PinField ZERO_PINB6 = (1L << pinPort(1, 6));
        const PinField ZERO_PINB7 = (1L << pinPort(1, 7));
    #endif

    #ifdef PORTC
        const PinField ZERO_PINC0 = (1L << pinPort(2, 0));
        const PinField ZERO_PINC1 = (1L << pinPort(2, 1));
        const PinField ZERO_PINC2 = (1L << pinPort(2, 2));
        const PinField ZERO_PINC3 = (1L << pinPort(2, 3));
        const PinField ZERO_PINC4 = (1L << pinPort(2, 4));
        const PinField ZERO_PINC5 = (1L << pinPort(2, 5));
        const PinField ZERO_PINC6 = (1L << pinPort(2, 6));
        const PinField ZERO_PINC7 = (1L << pinPort(2, 7));
    #endif

    #ifdef PORTD
        const PinField ZERO_PIND0 = (1L << pinPort(3, 0));
        const PinField ZERO_PIND1 = (1L << pinPort(3, 1));
        const PinField ZERO_PIND2 = (1L << pinPort(3, 2));
        const PinField ZERO_PIND3 = (1L << pinPort(3, 3));
        const PinField ZERO_PIND4 = (1L << pinPort(3, 4));
        const PinField ZERO_PIND5 = (1L << pinPort(3, 5));
        const PinField ZERO_PIND6 = (1L << pinPort(3, 6));
        const PinField ZERO_PIND7 = (1L << pinPort(3, 7));
    #endif


    class Gpio {
    public:
        // Life-cycle
        Gpio(const PinField pins);                      // claims ownership of a set of pins
        ~Gpio();                                        // frees the pins for re-use
        explicit operator bool() const;                 // validity checking

        // Management
        PinField getAllocatedPins() const;              // returns the pins owned by this Gpio

        // Control
        void reset() const;                             // tristates all owned pins

        void setAsInput() const;                        // sets all owned pins to inputs
        void setAsOutput() const;                       // sets all owned pins to outputs
        void switchOn() const;                          // sets all owned pins to high
        void switchOff() const;                         // sets all owned pins to low
        void toggle() const;                            // toggles all owned pins

        void setAsInput(const PinField pins) const;     // sets a subset of owned pins to inputs
        void setAsOutput(const PinField pins) const;    // sets a subset of owned pins to outputs
        void switchOn(const PinField pins) const;       // sets a subset of owned pins to high
        void switchOff(const PinField pins) const;      // sets a subset of owned pins to low
        void toggle(const PinField pins) const;         // toggles a subset of owned pins

        PinField getInputState() const;                 // Returns the input state of all owned pins

        #include "gpio_private.h"
    };

}


#endif


#endif
