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


#define PIN(port,pin) (((port) << 3) | ((pin) & 7))


namespace zero {


    typedef uint32_t PinField;


    #ifdef PORTA
        const PinField PF_PINA0 = (1L << PIN(0, 0));
        const PinField PF_PINA1 = (1L << PIN(0, 1));
        const PinField PF_PINA2 = (1L << PIN(0, 2));
        const PinField PF_PINA3 = (1L << PIN(0, 3));
        const PinField PF_PINA4 = (1L << PIN(0, 4));
        const PinField PF_PINA5 = (1L << PIN(0, 5));
        const PinField PF_PINA6 = (1L << PIN(0, 6));
        const PinField PF_PINA7 = (1L << PIN(0, 7));
    #endif

    #ifdef PORTB
        const PinField PF_PINB0 = (1L << PIN(1, 0));
        const PinField PF_PINB1 = (1L << PIN(1, 1));
        const PinField PF_PINB2 = (1L << PIN(1, 2));
        const PinField PF_PINB3 = (1L << PIN(1, 3));
        const PinField PF_PINB4 = (1L << PIN(1, 4));
        const PinField PF_PINB5 = (1L << PIN(1, 5));
        const PinField PF_PINB6 = (1L << PIN(1, 6));
        const PinField PF_PINB7 = (1L << PIN(1, 7));
    #endif

    #ifdef PORTC
        const PinField PF_PINC0 = (1L << PIN(2, 0));
        const PinField PF_PINC1 = (1L << PIN(2, 1));
        const PinField PF_PINC2 = (1L << PIN(2, 2));
        const PinField PF_PINC3 = (1L << PIN(2, 3));
        const PinField PF_PINC4 = (1L << PIN(2, 4));
        const PinField PF_PINC5 = (1L << PIN(2, 5));
        const PinField PF_PINC6 = (1L << PIN(2, 6));
        const PinField PF_PINC7 = (1L << PIN(2, 7));
    #endif

    #ifdef PORTD
        const PinField PF_PIND0 = (1L << PIN(3, 0));
        const PinField PF_PIND1 = (1L << PIN(3, 1));
        const PinField PF_PIND2 = (1L << PIN(3, 2));
        const PinField PF_PIND3 = (1L << PIN(3, 3));
        const PinField PF_PIND4 = (1L << PIN(3, 4));
        const PinField PF_PIND5 = (1L << PIN(3, 5));
        const PinField PF_PIND6 = (1L << PIN(3, 6));
        const PinField PF_PIND7 = (1L << PIN(3, 7));
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
