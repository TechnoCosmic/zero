//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_GPIO


#include <stdint.h>
#include <avr/io.h>
#include <util/atomic.h>
#include "gpio.h"


using namespace zero;


namespace {

    PinField _allocatedPins = 0ULL;

}


// ctor
Gpio::Gpio(const PinField pins) :
    _pins { [&]() -> PinField
    {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (_allocatedPins & pins) {
                return 0ULL;                            // pins could not be allocated
            }
            else {
                _allocatedPins |= pins;                 // add these to the allocated set
                return pins;                            // return valid pins to the ctor
            }
        }
    }()}
{
}


// dtor
Gpio::~Gpio()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _allocatedPins &= ~_pins;                       // free the pins
    }
}


// validity checking
Gpio::operator bool() const
{
    return (_pins != 0UL);
}


// Returns the pins currently owned by the Gpio object
PinField Gpio::getAllocatedPins() const
{
    return _pins;
}


// Sanitizes the pins by removing ones not allocated
inline PinField Gpio::sanitize(const PinField pins) const
{
    return pins & _pins;
}


// Tristates all owned pins
void Gpio::reset() const
{
    switchOff();
    setAsInput();
}


// Sets all of the owned pins to input
void Gpio::setAsInput() const
{
    setAsInput(_pins);
}


// Sets all of the owned pins to output
void Gpio::setAsOutput() const
{
    setAsOutput(_pins);
}


// Sets all of the owned pins to high
void Gpio::switchOn() const
{
    switchOn(_pins);
}


// Sets all of the owned pins to low
void Gpio::switchOff() const
{
    switchOff(_pins);
}


// Toggles all of the owned pins
void Gpio::toggle() const
{
    toggle(_pins);
}


// Sets a given set of owned pins to input
void Gpio::setAsInput(const PinField pins) const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const auto cleanPins = ~sanitize(pins);

        #ifdef DDRA
            DDRA &= ((cleanPins >>  0) & 0xFF);
        #endif

        #ifdef DDRB
            DDRB &= ((cleanPins >>  8) & 0xFF);
        #endif

        #ifdef DDRC
            DDRC &= ((cleanPins >> 16) & 0xFF);
        #endif

        #ifdef DDRD
            DDRD &= ((cleanPins >> 24) & 0xFF);
        #endif
    }
}


// Sets a given set of owned pins to output
void Gpio::setAsOutput(const PinField pins) const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const auto cleanPins = sanitize(pins);

        #ifdef DDRA
            DDRA |= ((cleanPins >>  0) & 0xFF);
        #endif

        #ifdef DDRB
            DDRB |= ((cleanPins >>  8) & 0xFF);
        #endif

        #ifdef DDRC
            DDRC |= ((cleanPins >> 16) & 0xFF);
        #endif

        #ifdef DDRD
            DDRD |= ((cleanPins >> 24) & 0xFF);
        #endif
    }
}


// Sets a given set of owned pins to high
void Gpio::switchOn(const PinField pins) const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const auto cleanPins = sanitize(pins);

        #ifdef PORTA
            PORTA |= ((cleanPins >>  0) & 0xFF);
        #endif

        #ifdef PORTB
            PORTB |= ((cleanPins >>  8) & 0xFF);
        #endif

        #ifdef PORTC
            PORTC |= ((cleanPins >> 16) & 0xFF);
        #endif

        #ifdef PORTD
            PORTD |= ((cleanPins >> 24) & 0xFF);
        #endif
    }
}


// Sets a given set of owned pins to low
void Gpio::switchOff(const PinField pins) const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const auto cleanPins = ~sanitize(pins);

        #ifdef PORTA
            PORTA &= ((cleanPins >>  0) & 0xFF);
        #endif

        #ifdef PORTB
            PORTB &= ((cleanPins >>  8) & 0xFF);
        #endif

        #ifdef PORTC
            PORTC &= ((cleanPins >> 16) & 0xFF);
        #endif

        #ifdef PORTD
            PORTD &= ((cleanPins >> 24) & 0xFF);
        #endif
    }
}


// Toggles a given set of owned pins
void Gpio::toggle(const PinField pins) const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const auto cleanPins = sanitize(pins);

        #ifdef PORTA
            PORTA ^= ((cleanPins >>  0) & 0xFF);
        #endif

        #ifdef PORTB
            PORTB ^= ((cleanPins >>  8) & 0xFF);
        #endif

        #ifdef PORTC
            PORTC ^= ((cleanPins >> 16) & 0xFF);
        #endif

        #ifdef PORTD
            PORTD ^= ((cleanPins >> 24) & 0xFF);
        #endif
    }
}


// Returns the input state of all owned pins
PinField Gpio::getInputState() const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        PinField rc = 0ULL;

        #ifdef PINA
            rc |= (((PinField) PINA) <<  0);
        #endif

        #ifdef PINB
            rc |= (((PinField) PINB) <<  8);
        #endif

        #ifdef PINC
            rc |= (((PinField) PINC) << 16);
        #endif

        #ifdef PIND
            rc |= (((PinField) PIND) << 24);
        #endif

        return sanitize(rc);
    }
}


// Returns the output state of all owned pins
PinField Gpio::getOutputState() const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        PinField rc = 0ULL;

        #ifdef PORTA
            rc |= (((PinField) PORTA) <<  0);
        #endif

        #ifdef PORTB
            rc |= (((PinField) PORTB) <<  8);
        #endif

        #ifdef PORTC
            rc |= (((PinField) PORTC) << 16);
        #endif

        #ifdef PORTD
            rc |= (((PinField) PORTD) << 24);
        #endif

        return sanitize(rc);
    }
}


// Sets all owned pins' outputs according the supplied value
void Gpio::setOutputState(const PinField v) const
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const PinField cleanPins = sanitize(v);

        #ifdef PORTA
        {
            const uint8_t alloc = (_pins >> 0) & 0xFF;
            const uint8_t incoming = (v >> 0) & 0xFF;
            PORTA = (PORTA & ~alloc) | ((cleanPins >> 0) & 0xFF);
        }
        #endif

        #ifdef PORTB
        {
            const uint8_t alloc = (_pins >> 8) & 0xFF;
            const uint8_t incoming = (v >> 8) & 0xFF;
            PORTB = (PORTB & ~alloc) | ((cleanPins >> 8) & 0xFF);
        }
        #endif

        #ifdef PORTC
        {
            const uint8_t alloc = (_pins >> 16) & 0xFF;
            const uint8_t incoming = (v >> 16) & 0xFF;
            PORTC = (PORTC & ~alloc) | ((cleanPins >> 16) & 0xFF);
        }
        #endif

        #ifdef PORTD
        {
            const uint8_t alloc = (_pins >> 24) & 0xFF;
            const uint8_t incoming = (v >> 24) & 0xFF;
            PORTD = (PORTD & ~alloc) | ((cleanPins >> 24) & 0xFF);
        }
        #endif
    }
}


#endif
