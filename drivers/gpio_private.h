//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


public:
    /// @privatesection
    static void init();

    static void handlePinChange(
        const uint8_t portNumber,                       // 0 = PINA, 1 = PINB etc
        const uint8_t v );                              // the current value of PINx

    // lifecycle
    ~Gpio();                                            // frees the pins for re-use

    // list membership
    Gpio* _prev;
    Gpio* _next;

private:
    Gpio(
        const PinField pins,                            // pins to which you want exclusive access
        const InputCallback c,                          // optional callback when input pins change state
        const Synapse* syn );                           // optional Synapse to signal when input pins change state

    static void setInterrupts( const PinField pins );   // sets the on/off state of all PCINTs
    static PinField gatherAllInputPins();               // returns all input pins across all ports

    inline PinField sanitize( const PinField pins ) const;

    const InputCallback _inputCallback;                 // called when an input pin changes state
    const Synapse* _inputSynapse;                       // signalled when an input pin changes state
    const PinField _pins;                               // pins owned by this object

    PinControl _directionControl;                       // Determines if changes to the direction are allowed
    PinControl _outputControl;                          // Determines if changes to the high/low state are allowed
