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
#include "thread.h"


namespace zero {

    // class decl because chicken/egg
    class Gpio;

    typedef uint32_t PinField;
    typedef void ( *InputCallback )( const Gpio& pins );


    // Maps a port number and pin number to a simple integer
    constexpr uint32_t pinPort( const uint8_t port, const uint8_t pin )
    {
        return ( (uint32_t) port << 3 ) | ( (uint32_t) pin & 7 );
    }


    // clang-format off
    #ifdef PORTD
        const PinField ZERO_PIND0 = ( 1L << pinPort( 3, 0 ) );
        const PinField ZERO_PIND1 = ( 1L << pinPort( 3, 1 ) );
        const PinField ZERO_PIND2 = ( 1L << pinPort( 3, 2 ) );
        const PinField ZERO_PIND3 = ( 1L << pinPort( 3, 3 ) );
        const PinField ZERO_PIND4 = ( 1L << pinPort( 3, 4 ) );
        const PinField ZERO_PIND5 = ( 1L << pinPort( 3, 5 ) );
        const PinField ZERO_PIND6 = ( 1L << pinPort( 3, 6 ) );
        const PinField ZERO_PIND7 = ( 1L << pinPort( 3, 7 ) );

        #ifndef ZERO_HIGH_PORT
            #define ZERO_HIGH_PORT 3
        #endif
    #endif

    #ifdef PORTC
        const PinField ZERO_PINC0 = ( 1L << pinPort( 2, 0 ) );
        const PinField ZERO_PINC1 = ( 1L << pinPort( 2, 1 ) );
        const PinField ZERO_PINC2 = ( 1L << pinPort( 2, 2 ) );
        const PinField ZERO_PINC3 = ( 1L << pinPort( 2, 3 ) );
        const PinField ZERO_PINC4 = ( 1L << pinPort( 2, 4 ) );
        const PinField ZERO_PINC5 = ( 1L << pinPort( 2, 5 ) );
        const PinField ZERO_PINC6 = ( 1L << pinPort( 2, 6 ) );
        const PinField ZERO_PINC7 = ( 1L << pinPort( 2, 7 ) );

        #ifndef ZERO_HIGH_PORT
            #define ZERO_HIGH_PORT 2
        #endif
    #endif

    #ifdef PORTB
        const PinField ZERO_PINB0 = ( 1L << pinPort( 1, 0 ) );
        const PinField ZERO_PINB1 = ( 1L << pinPort( 1, 1 ) );
        const PinField ZERO_PINB2 = ( 1L << pinPort( 1, 2 ) );
        const PinField ZERO_PINB3 = ( 1L << pinPort( 1, 3 ) );
        const PinField ZERO_PINB4 = ( 1L << pinPort( 1, 4 ) );
        const PinField ZERO_PINB5 = ( 1L << pinPort( 1, 5 ) );
        const PinField ZERO_PINB6 = ( 1L << pinPort( 1, 6 ) );
        const PinField ZERO_PINB7 = ( 1L << pinPort( 1, 7 ) );

        #ifndef ZERO_HIGH_PORT
            #define ZERO_HIGH_PORT 1
        #endif
    #endif

    #ifdef PORTA
        const PinField ZERO_PINA0 = ( 1L << pinPort( 0, 0 ) );
        const PinField ZERO_PINA1 = ( 1L << pinPort( 0, 1 ) );
        const PinField ZERO_PINA2 = ( 1L << pinPort( 0, 2 ) );
        const PinField ZERO_PINA3 = ( 1L << pinPort( 0, 3 ) );
        const PinField ZERO_PINA4 = ( 1L << pinPort( 0, 4 ) );
        const PinField ZERO_PINA5 = ( 1L << pinPort( 0, 5 ) );
        const PinField ZERO_PINA6 = ( 1L << pinPort( 0, 6 ) );
        const PinField ZERO_PINA7 = ( 1L << pinPort( 0, 7 ) );

        #ifndef ZERO_HIGH_PORT
            #define ZERO_HIGH_PORT 0
        #endif
    #endif
    // clang-format on


    const int ZERO_NUM_PORTS = ZERO_HIGH_PORT + 1;


    /// @brief Determines whether future changes to the pins are permitted
    enum class PinControl {
        Free = 0,
        Locked
    };


    /// @brief An attribute of GPIO
    enum GpioAspect {
        Direction = (1 << 0 ),
        Io = (1 << 1 ),
    };


    /// @brief Provides protected access to GPIO pins
    class Gpio {
    public:
        // lifecycle
        Gpio( const PinField pins );                    // pins to which you want exclusive access

        Gpio(
            const PinField pins,                        // pins to which you want exclusive access
            const InputCallback c );                    // callback for when input pins change state

        Gpio(
            const PinField pins,                        // pins to which you want exclusive access
            const Synapse& syn );                       // Synapse to signal when input pins change state

        explicit operator bool() const;                 // validity checking

        // Management
        PinField getAllocatedPins() const;              // returns the pins owned by this Gpio

        // Control
        void reset();                                   // tristates all owned pins

        void setAsInput();                              // sets all owned pins to inputs
        void setAsOutput();                             // sets all owned pins to outputs
        void switchOn();                                // sets all owned pins to high
        void switchOff();                               // sets all owned pins to low
        void toggle() const;                            // toggles all owned pins

        void setAsInput( const PinField pins );         // sets a subset of owned pins to inputs
        void setAsOutput( const PinField pins );        // sets a subset of owned pins to outputs
        void switchOn( const PinField pins );           // sets a subset of owned pins to high
        void switchOff( const PinField pins );          // sets a subset of owned pins to low
        void toggle( const PinField pins ) const;       // toggles a subset of owned pins

        uint32_t getInputState() const;                 // Returns the input state of all owned pins
        uint32_t getOutputState() const;                // Returns the output state of all owned pins

        void setOutputState(
            const uint32_t v,
            const PinControl lock = PinControl::Free ); // Sets the output states of all owned pins

        void lock( const GpioAspect a );                // prevents further changes to some aspect of the Gpio

        #include "gpio_private.h"
    };

}    // namespace zero


#endif


#endif
