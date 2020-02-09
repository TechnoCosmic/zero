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

#include "list.h"
#include "gpio.h"


using namespace zero;


namespace {


    #ifdef PORTA
        #define PC_PORTA_vect PCINT0_vect
        #define PC_PORTB_vect PCINT1_vect
        #define PC_PORTC_vect PCINT2_vect
        #define PC_PORTD_vect PCINT3_vect

        #define PCMSKA PCMSK0
        #define PCMSKB PCMSK1
        #define PCMSKC PCMSK2
        #define PCMSKD PCMSK3
        
        #define PCPA 0
        #define PCPB 1
        #define PCPC 2
        #define PCPD 3
        
    #else
        #ifdef PORTB
            #define PC_PORTB_vect PCINT0_vect
            #define PC_PORTC_vect PCINT1_vect
            #define PC_PORTD_vect PCINT2_vect

            #define PCMSKB PCMSK0
            #define PCMSKC PCMSK1
            #define PCMSKD PCMSK2
        
            #define PCPB 0
            #define PCPC 1
            #define PCPD 2
        
        #else
            #define PC_PORTC_vect PCINT0_vect
            #define PC_PORTD_vect PCINT1_vect

            #define PCMSKC PCMSK0
            #define PCMSKD PCMSK1
            
            #define PCPC 0
            #define PCPD 1
        #endif
    #endif


    PinField _allocatedPins = 0ULL;                     // for quick determination of available pins
    List<Gpio> _gpioList;                               // for PCINTs
    uint8_t _lastKnownInputs[] = {                      // pin change state tracking
        0, 0, 0, 0,
    };


}


// tri-state all pins and get the initial input state
void Gpio::init()
{
    #ifdef DDRA
        DDRA = 0;
        PORTA = 0;
        _lastKnownInputs[0] = PINA;
    #endif

    #ifdef DDRB
        DDRB = 0;
        PORTB = 0;
        _lastKnownInputs[1] = PINB;
    #endif

    #ifdef DDRC
        DDRC = 0;
        PORTC = 0;
        _lastKnownInputs[2] = PINC;
    #endif

    #ifdef DDRD
        DDRD = 0;
        PORTD = 0;
        _lastKnownInputs[3] = PIND;
    #endif
}


// ctor
Gpio::Gpio(
    const PinField pins)
:
    Gpio( pins, nullptr, nullptr )
{
}


// ctor
Gpio::Gpio(
    const PinField pins,
    const InputCallback c)
:
    Gpio( pins, c, nullptr )
{
}


// ctor
Gpio::Gpio(
    const PinField pins,
    const Synapse& syn)
:
    Gpio( pins, nullptr, &syn )
{
}


// ctor
Gpio::Gpio(
    const PinField pins,
    const InputCallback c,
    const Synapse* syn)
:
    _prev{ nullptr },
    _next{ nullptr },
    _inputCallback{ c },
    _inputSynapse{ syn },
    _pins { [&]() -> PinField
    {
        ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
            if (_allocatedPins & pins) {
                return 0ULL;                            // pins could not be allocated
            }
            else {
                _allocatedPins |= pins;                 // add these to the quick set
                _gpioList.append( *this );
                return pins;                            // return valid pins to the ctor
            }
        }
    }()}
{
}


// dtor
Gpio::~Gpio()
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        _allocatedPins &= ~_pins;                       // free the pins
        _gpioList.remove( *this );
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
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        switchOff();
        setAsInput();
    }
}


// Sets all of the owned pins to input
void Gpio::setAsInput() const
{
    setAsInput( _pins );
}


// Sets all of the owned pins to output
void Gpio::setAsOutput() const
{
    setAsOutput( _pins );
}


// Sets all of the owned pins to high
void Gpio::switchOn() const
{
    switchOn( _pins );
}


// Sets all of the owned pins to low
void Gpio::switchOff() const
{
    switchOff( _pins );
}


// Toggles all of the owned pins
void Gpio::toggle() const
{
    toggle( _pins );
}


// Sets a given set of owned pins to input
void Gpio::setAsInput(const PinField pins) const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        const auto cleanPins = ~sanitize( pins );

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

        // re-assess which pins are subject to PCINTs
        Gpio::setInterrupts( Gpio::gatherAllInputPins() );
    }
}


// Sets a given set of owned pins to output
void Gpio::setAsOutput(const PinField pins) const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        const auto cleanPins = sanitize( pins );

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

        // re-assess which pins are subject to PCINTs
        Gpio::setInterrupts( Gpio::gatherAllInputPins() );
    }
}


// Sets a given set of owned pins to high
void Gpio::switchOn(const PinField pins) const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        const auto cleanPins = sanitize( pins );

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
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        const auto cleanPins = ~sanitize( pins );

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
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        const auto cleanPins = sanitize( pins );

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
uint32_t Gpio::getInputState() const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        // fetch all input values ASAP so that we
        // can get as close to an 'atomic' a snapshot
        // of the inputs as possible
        #ifdef PINA
            const uint8_t pina = PINA;
        #endif

        #ifdef PINB
            const uint8_t pinb = PINB;
        #endif

        #ifdef PINC
            const uint8_t pinc = PINC;
        #endif

        #ifdef PIND
            const uint8_t pind = PIND;
        #endif

        // now merge them into the [almost] final PinField
        PinField rc = 0ULL;

        #ifdef PINA
            rc |= (((PinField) pina) <<  0);
        #endif

        #ifdef PINB
            rc |= (((PinField) pinb) <<  8);
        #endif

        #ifdef PINC
            rc |= (((PinField) pinc) << 16);
        #endif

        #ifdef PIND
            rc |= (((PinField) pind) << 24);
        #endif

        return sanitize( rc );
    }
}


// Returns the output state of all owned pins
uint32_t Gpio::getOutputState() const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
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

        return sanitize( rc );
    }
}


// Sets the output state of all owned pins
void Gpio::setOutputState(const uint32_t v) const
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        const PinField cleanPins = sanitize( v );

        #ifdef PORTA
            const uint8_t allocA = (_pins >> 0) & 0xFF;
            const uint8_t incomingA = (cleanPins >> 0) & 0xFF;
            PORTA = (PORTA & ~allocA) | incomingA;
        #endif

        #ifdef PORTB
            const uint8_t allocB = (_pins >> 8) & 0xFF;
            const uint8_t incomingB = (cleanPins >> 8) & 0xFF;
            PORTB = (PORTB & ~allocB) | incomingB;
        #endif

        #ifdef PORTC
            const uint8_t allocC = (_pins >> 16) & 0xFF;
            const uint8_t incomingC = (cleanPins >> 16) & 0xFF;
            PORTC = (PORTC & ~allocC) | incomingC;
        #endif

        #ifdef PORTD
            const uint8_t allocD = (_pins >> 24) & 0xFF;
            const uint8_t incomingD = (cleanPins >> 24) & 0xFF;
            PORTD = (PORTD & ~allocD) | incomingD;
        #endif
    }
}


// sets the on/off state of all PCINTs
void Gpio::setInterrupts(const PinField pins)
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        // for each port, set the PC masks, and then either
        // enable or disable the ISR for that port accordingly
        #ifdef PCMSKA
            PCMSKA = (pins >>  0) & 0xFF;

            if (PCMSKA) {
                PCICR |= (1 << PCPA);
            }
            else {
                PCICR &= ~(1 << PCPA);
            }
        #endif

        #ifdef PCMSKB
            PCMSKB = (pins >>  8) & 0xFF;

            if (PCMSKB) {
                PCICR |= (1 << PCPB);
            }
            else {
                PCICR &= ~(1 << PCPB);
            }
        #endif

        #ifdef PCMSKC
            PCMSKC = (pins >> 16) & 0xFF;

            if (PCMSKC) {
                PCICR |= (1 << PCPC);
            }
            else {
                PCICR &= ~(1 << PCPC);
            }
        #endif

        #ifdef PCMSKD
            PCMSKD = (pins >> 24) & 0xFF;

            if (PCMSKD) {
                PCICR |= (1 << PCPD);
            }
            else {
                PCICR &= ~(1 << PCPD);
            }
        #endif
    }
}


// Determines which pins are inputs across all Gpio objects
PinField Gpio::gatherAllInputPins()
{
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        PinField rc = 0ULL;

        #ifdef DDRA
            const uint8_t inputsA = ~DDRA;
            rc |= (((PinField) inputsA) <<  0);
        #endif

        #ifdef DDRB
            const uint8_t inputsB = ~DDRB;
            rc |= (((PinField) inputsB) <<  8);
        #endif

        #ifdef DDRC
            const uint8_t inputsC = ~DDRC;
            rc |= (((PinField) inputsC) << 16);
        #endif

        #ifdef DDRD
            const uint8_t inputsD = ~DDRD;
            rc |= (((PinField) inputsD) << 24);
        #endif

        return rc;
    }
}


// Dispatches the pin change interrupt to the correct Gpio objects
void Gpio::handlePinChange(const int portNumber, const uint8_t newValue)
{
    // figure out which pins actually changed
    const PinField changedInPort = newValue ^ _lastKnownInputs[portNumber];
    const PinField changedField = changedInPort << (portNumber << 3);

    _lastKnownInputs[portNumber] = newValue;

    // find out who we need to notify
    Gpio* cur = _gpioList.getHead();
    PinField remaining = changedField;

    while (remaining && cur) {
        const PinField curPins = cur->getAllocatedPins();

        if (curPins & remaining) {
            remaining &= ~curPins;

            // always call the callback first
            if (cur->_inputCallback) {
                cur->_inputCallback();
            }

            // .. and only then the Synapse
            if (cur->_inputSynapse) {
                cur->_inputSynapse->signal();
            }
        }
        else {
            cur = cur->_next;
        }
    }
}


#ifdef PC_PORTA_vect
    ISR(PC_PORTA_vect)
    {
        Gpio::handlePinChange( 0, PINA );
    }
#endif


#ifdef PC_PORTB_vect
    ISR(PC_PORTB_vect)
    {
        Gpio::handlePinChange( 1, PINB );
    }
#endif


#ifdef PC_PORTC_vect
    ISR(PC_PORTC_vect)
    {
        Gpio::handlePinChange( 2, PINC );
    }
#endif


#ifdef PC_PORTD_vect
    ISR(PC_PORTD_vect)
    {
        Gpio::handlePinChange( 3, PIND );
    }
#endif


#endif
