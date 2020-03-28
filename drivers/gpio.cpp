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


    PinField _allocatedPins{ 0UL };                     // for quick determination of available pins
    List<Gpio> _gpioList;                               // for PCINTs
    uint8_t _lastKnownInputs[] = {
        // pin change state tracking
        0,
        0,
        0,
        0,
    };

}    // namespace


// tri-state all pins and get the initial input state
void Gpio::init()
{
    #ifdef DDRA
        DDRA = 0;
        PORTA = 0;
        _lastKnownInputs[ 0 ] = PINA;
    #endif

    #ifdef DDRB
        DDRB = 0;
        PORTB = 0;
        _lastKnownInputs[ 1 ] = PINB;
    #endif

    #ifdef DDRC
        DDRC = 0;
        PORTC = 0;
        _lastKnownInputs[ 2 ] = PINC;
    #endif

    #ifdef DDRD
        DDRD = 0;
        PORTD = 0;
        _lastKnownInputs[ 3 ] = PIND;
    #endif
}


/// @brief Gains exclusive access to one or more GPIO pins
/// @param pins The GPIO pins for which exclusive access is required.
/// @note Initialization of the Gpio object will fail if another Gpio is using any of the
/// pins. Check for this before using the Gpio object.
/// @code
/// int gpioDemo()
/// {
///     Gpio ledPins{ ZERO_PINC0 | ZERO_PINC1 };
///
///     if ( ledPins ) {
///         // do things with the pins
///         ledPins.switchOn();
///     }
/// }
/// @endcode
Gpio::Gpio(
    const PinField pins)
:
    Gpio( pins, nullptr, nullptr )
{
    // empty
}


/// @brief Gains exclusive access to one or more GPIO pins
/// @param pins The GPIO pins for which exclusive access is required.
/// @param c A pointer to a function that will be run when any of the input pins in the
/// Gpio change state.
/// @note Initialization of the Gpio object will fail if another Gpio is using any of the
/// pins. Check for this before using the Gpio object.
/// @code
/// void myPinChangeHandler( const Gpio& pins )
/// {
///     dbg_pgm( "Pin C0 changed!\r\n" );
/// }
///
/// int gpioDemo()
/// {
///     Gpio ledPins{ ZERO_PINC0 | ZERO_PINC1 };
///
///     if ( ledPins ) {
///         ledPins.setAsInput( ZERO_PINC0 );           // set C0 as input
///         ledPins.switchOn( ZERO_PINC0 );             // enable pull-up resistor
///
///         // do things with the pins
///         ledPins.switchOn( ZERO_PINC1 );
///     }
/// }
/// @endcode
Gpio::Gpio(
    const PinField pins,
    const InputCallback c)
:
    Gpio( pins, c, nullptr )
{
    // empty
}


/// @brief Gains exclusive access to one or more GPIO pins
/// @param pins The GPIO pins for which exclusive access is required.
/// @param syn The Synapse to signal when any of the pins' input states change.
/// @note Initialization of the Gpio object will fail if another Gpio is using any of the
/// pins. Check for this before using the Gpio object.
/// @code
/// int gpioDemo()
/// {
///     Synapse pinChangeSyn;
///     Gpio ledPins{ ZERO_PINC0 | ZERO_PINC1, pinChangeSyn };
///
///     if ( ledPins ) {
///         ledPins.setAsInput( ZERO_PINC0 );           // set C0 as input
///         ledPins.switchOn( ZERO_PINC0 );             // enable pull-up resistor
///
///         while ( true ) {
///             const auto recdSignals{ pinChangeSyn.wait() };
///
///             if ( recdSignals & pinChangeSyn ) {
///                 dbg_pgm( "Pin C0 changed!\r\n" );
///             }
///         }
///     }
/// }
/// @endcode
Gpio::Gpio(
    const PinField pins,
    const Synapse& syn)
:
    Gpio( pins, nullptr, &syn )
{
    // empty
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
    _pins{ [&]() -> PinField
    {
        ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
            if ( _allocatedPins & pins ) {
                return 0UL;                             // pins could not be allocated
            }
            else {
                _allocatedPins |= pins;                 // add these to the quick set
                _gpioList.append( *this );
                return pins;                            // return valid pins to the ctor
            }
        }
    }() }
{
    // empty
}


// dtor
Gpio::~Gpio()
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        _allocatedPins &= ~_pins;                       // free the pins
        _gpioList.remove( *this );
    }
}


/// @brief Determines if the Gpio object initialized correctly
/// @returns ```true``` if the object initialized correctly, ```false``` otherwise.
Gpio::operator bool() const
{
    return _pins;
}


/// @brief Returns the pins currently owned by the Gpio object
/// @returns A PinField containing the pins owned by the object.
PinField Gpio::getAllocatedPins() const
{
    return _pins;
}


// Sanitizes the pins by removing ones not allocated
inline PinField Gpio::sanitize( const PinField pins ) const
{
    return pins & _pins;
}


/// @brief Tristates all owned pins
void Gpio::reset() const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        switchOff();
        setAsInput();
    }
}


/// @brief Sets all of the owned pins to input
void Gpio::setAsInput() const
{
    setAsInput( _pins );
}


/// @brief Sets all of the owned pins to output
void Gpio::setAsOutput() const
{
    setAsOutput( _pins );
}


/// @brief Sets all owned pins to high/on
void Gpio::switchOn() const
{
    switchOn( _pins );
}


/// @brief Sets all owned pins to low/off
void Gpio::switchOff() const
{
    switchOff( _pins );
}


/// @brief Toggles all owned pins
void Gpio::toggle() const
{
    toggle( _pins );
}


/// @brief Sets a given subset of owned pins to input
/// @param pins A PinField specifing the subset of pins to set as inputs.
void Gpio::setAsInput( const PinField pins ) const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        const auto cleanPins{ ~sanitize( pins ) };

        #ifdef DDRA
            DDRA &= ( ( cleanPins >> 0 ) & 0xFF );
        #endif

        #ifdef DDRB
            DDRB &= ( ( cleanPins >> 8 ) & 0xFF );
        #endif

        #ifdef DDRC
            DDRC &= ( ( cleanPins >> 16 ) & 0xFF );
        #endif

        #ifdef DDRD
            DDRD &= ( ( cleanPins >> 24 ) & 0xFF );
        #endif

        // re-assess which pins are subject to PCINTs
        Gpio::setInterrupts( Gpio::gatherAllInputPins() );
    }
}


/// @brief Sets a given subset of owned pins to output
/// @param pins A PinField specifing the subset of pins to set as outputs.
void Gpio::setAsOutput( const PinField pins ) const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        const auto cleanPins{ sanitize( pins ) };

        #ifdef DDRA
            DDRA |= ( ( cleanPins >> 0 ) & 0xFF );
        #endif

        #ifdef DDRB
            DDRB |= ( ( cleanPins >> 8 ) & 0xFF );
        #endif

        #ifdef DDRC
            DDRC |= ( ( cleanPins >> 16 ) & 0xFF );
        #endif

        #ifdef DDRD
            DDRD |= ( ( cleanPins >> 24 ) & 0xFF );
        #endif

        // re-assess which pins are subject to PCINTs
        Gpio::setInterrupts( Gpio::gatherAllInputPins() );
    }
}


/// @brief Sets a given subset of owned pins to high/on
/// @param pins A PinField specifing the subset of pins to switch on.
void Gpio::switchOn( const PinField pins ) const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        const auto cleanPins{ sanitize( pins ) };

        #ifdef PORTA
            PORTA |= ( ( cleanPins >> 0 ) & 0xFF );
        #endif

        #ifdef PORTB
            PORTB |= ( ( cleanPins >> 8 ) & 0xFF );
        #endif

        #ifdef PORTC
            PORTC |= ( ( cleanPins >> 16 ) & 0xFF );
        #endif

        #ifdef PORTD
            PORTD |= ( ( cleanPins >> 24 ) & 0xFF );
        #endif
    }
}


/// @brief Sets a given subset of owned pins to low/off
/// @param pins A PinField specifing the subset of pins to switch off.
void Gpio::switchOff( const PinField pins ) const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        const auto cleanPins{ ~sanitize( pins ) };

        #ifdef PORTA
            PORTA &= ( ( cleanPins >> 0 ) & 0xFF );
        #endif

        #ifdef PORTB
            PORTB &= ( ( cleanPins >> 8 ) & 0xFF );
        #endif

        #ifdef PORTC
            PORTC &= ( ( cleanPins >> 16 ) & 0xFF );
        #endif

        #ifdef PORTD
            PORTD &= ( ( cleanPins >> 24 ) & 0xFF );
        #endif
    }
}


/// @brief Toggles a given subset of owned pins
/// @param pins A PinField specifing the subset of pins to toggle.
void Gpio::toggle( const PinField pins ) const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        const auto cleanPins{ sanitize( pins ) };

        #ifdef PORTA
            PORTA ^= ( ( cleanPins >> 0 ) & 0xFF );
        #endif

        #ifdef PORTB
            PORTB ^= ( ( cleanPins >> 8 ) & 0xFF );
        #endif

        #ifdef PORTC
            PORTC ^= ( ( cleanPins >> 16 ) & 0xFF );
        #endif

        #ifdef PORTD
            PORTD ^= ( ( cleanPins >> 24 ) & 0xFF );
        #endif
    }
}


/// @brief Returns the input states of all owned pins
/// @returns A ```uint32_t``` reflecting the input states of all owned pins.
/// @note Pins that are not owned by the Gpio will have their corresponding bits set to
/// zero (0).
uint32_t Gpio::getInputState() const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
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
        PinField rc{ 0UL };

        #ifdef PINA
            rc |= ( ( (PinField) pina ) << 0 );
        #endif

        #ifdef PINB
            rc |= ( ( (PinField) pinb ) << 8 );
        #endif

        #ifdef PINC
            rc |= ( ( (PinField) pinc ) << 16 );
        #endif

        #ifdef PIND
            rc |= ( ( (PinField) pind ) << 24 );
        #endif

        return sanitize( rc );
    }
}


/// @brief Returns the output states of all owned pins
/// @returns A ```uint32_t``` reflecting the output states of all owned pins.
/// @note Pins that are not owned by the Gpio will have their corresponding bits set to
/// zero (0).
uint32_t Gpio::getOutputState() const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        PinField rc{ 0UL };

        #ifdef PORTA
            rc |= ( ( (PinField) PORTA ) << 0 );
        #endif

        #ifdef PORTB
            rc |= ( ( (PinField) PORTB ) << 8 );
        #endif

        #ifdef PORTC
            rc |= ( ( (PinField) PORTC ) << 16 );
        #endif

        #ifdef PORTD
            rc |= ( ( (PinField) PORTD ) << 24 );
        #endif

        return sanitize( rc );
    }
}


/// @brief Sets the output state of all owned pins
/// @param v A ```uint32_t``` representing the desired output states of all owned pins.
/// @note For each pin owned by the Gpio object, the state of that pin will be switched
/// off where it's corresponding bit is zero (0), and switched on where it's
/// corresponding bit is one (1). Unowned pins are not affected.
void Gpio::setOutputState( const uint32_t v ) const
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        const PinField cleanPins{ sanitize( v ) };

        #ifdef PORTA
            const uint8_t allocA = ( _pins >> 0 ) & 0xFF;
            const uint8_t incomingA = ( cleanPins >> 0 ) & 0xFF;
            PORTA = ( PORTA & ~allocA ) | incomingA;
        #endif

        #ifdef PORTB
            const uint8_t allocB = ( _pins >> 8 ) & 0xFF;
            const uint8_t incomingB = ( cleanPins >> 8 ) & 0xFF;
            PORTB = ( PORTB & ~allocB ) | incomingB;
        #endif

        #ifdef PORTC
            const uint8_t allocC = ( _pins >> 16 ) & 0xFF;
            const uint8_t incomingC = ( cleanPins >> 16 ) & 0xFF;
            PORTC = ( PORTC & ~allocC ) | incomingC;
        #endif

        #ifdef PORTD
            const uint8_t allocD = ( _pins >> 24 ) & 0xFF;
            const uint8_t incomingD = ( cleanPins >> 24 ) & 0xFF;
            PORTD = ( PORTD & ~allocD ) | incomingD;
        #endif
    }
}


// sets the on/off state of all PCINTs
void Gpio::setInterrupts( const PinField pins )
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        // for each port, set the PC masks, and then either
        // enable or disable the ISR for that port accordingly
        #ifdef PCMSKA
            PCMSKA = ( pins >> 0 ) & 0xFF;

            if ( PCMSKA ) {
                PCICR |= ( 1 << PCPA );
            }
            else {
                PCICR &= ~( 1 << PCPA );
            }
        #endif

        #ifdef PCMSKB
            PCMSKB = ( pins >> 8 ) & 0xFF;

            if ( PCMSKB ) {
                PCICR |= ( 1 << PCPB );
            }
            else {
                PCICR &= ~( 1 << PCPB );
            }
        #endif

        #ifdef PCMSKC
            PCMSKC = ( pins >> 16 ) & 0xFF;

            if ( PCMSKC ) {
                PCICR |= ( 1 << PCPC );
            }
            else {
                PCICR &= ~( 1 << PCPC );
            }
        #endif

        #ifdef PCMSKD
            PCMSKD = ( pins >> 24 ) & 0xFF;

            if ( PCMSKD ) {
                PCICR |= ( 1 << PCPD );
            }
            else {
                PCICR &= ~( 1 << PCPD );
            }
        #endif
    }
}


// Determines which pins are inputs across all Gpio objects
PinField Gpio::gatherAllInputPins()
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE ) {
        PinField rc{ 0UL };

        #ifdef DDRA
            const uint8_t inputsA = ~DDRA;
            rc |= ( ( (PinField) inputsA ) << 0 );
        #endif

        #ifdef DDRB
            const uint8_t inputsB = ~DDRB;
            rc |= ( ( (PinField) inputsB ) << 8 );
        #endif

        #ifdef DDRC
            const uint8_t inputsC = ~DDRC;
            rc |= ( ( (PinField) inputsC ) << 16 );
        #endif

        #ifdef DDRD
            const uint8_t inputsD = ~DDRD;
            rc |= ( ( (PinField) inputsD ) << 24 );
        #endif

        return rc;
    }
}


// Dispatches the pin change interrupt to the correct Gpio objects
void Gpio::handlePinChange( const uint8_t portNumber, const uint8_t newValue )
{
    // figure out which pins actually changed
    const PinField changedInPort{ (PinField) newValue ^ _lastKnownInputs[ portNumber ] };
    const PinField changedField{ changedInPort << ( portNumber << 3 ) };

    _lastKnownInputs[ portNumber ] = newValue;

    // find out who we need to notify
    Gpio* cur{ _gpioList.getHead() };
    PinField remaining{ changedField };

    while ( remaining and cur ) {
        const PinField curPins{ cur->getAllocatedPins() };

        if ( curPins & remaining ) {
            remaining &= ~curPins;

            // always call the callback first
            if ( cur->_inputCallback ) {
                cur->_inputCallback( *cur );
            }

            // .. and only then the Synapse
            if ( cur->_inputSynapse ) {
                cur->_inputSynapse->signal();
            }
        }
        else {
            cur = cur->_next;
        }
    }
}


#ifdef PC_PORTA_vect
ISR( PC_PORTA_vect )
{
    Gpio::handlePinChange( 0, PINA );
}
#endif


#ifdef PC_PORTB_vect
ISR( PC_PORTB_vect )
{
    Gpio::handlePinChange( 1, PINB );
}
#endif


#ifdef PC_PORTC_vect
ISR( PC_PORTC_vect )
{
    Gpio::handlePinChange( 2, PINC );
}
#endif


#ifdef PC_PORTD_vect
ISR( PC_PORTD_vect )
{
    Gpio::handlePinChange( 3, PIND );
}
#endif


#endif
