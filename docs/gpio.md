# Gpio
```zero/drivers/gpio.h```

The ```Gpio``` class provides protected access to the MCU's GPIO pins.

## Constructors
Constructs a new Gpio object, claiming exclusive access to one or more GPIO pins.

```
    Gpio::Gpio(
        const PinField pins
        )

    Gpio::Gpio(
        const PinField pins,
        const Synapse& syn
        )

    Gpio::Gpio(
        const PinField pins,
        const InputCallback cb
        )
```
### Parameters
|Param|Description|
|-----|-----------|
|```pins```|A 32-bit bitmap of the pins the object should 'own'.|
|```syn```|An optional ```Synapse``` that will be signalled when any of the ```Gpio```'s input pins changes state (pin change).|
|```cb```|An optional callback function that will be run when any of the ```Gpio```'s input pins changes state (pin change).|

### Notes
zero's GPIO model represents up to four (4) GPIO ports in a single ```PinField```, where ```PORTA0``` is bit 0, and ```PORTD7``` is bit 31. This is true even if your target MCU doesn't have a port, for example ```PORTA```, which isn't present on the ATmega328.

The ```pins``` parameter is a bitfield, allowing you to claim multiple pins in a single ```Gpio``` object. These pins can still be manipulated independently of each other by using methods of the ```Gpio``` class, or can be controlled as a group. Most methods, such as ```::setAsOutput()``` have a version that takes a ```PinField``` parameter, which allows you to specify a subset of the pins owned by the object. They will often also have a parameterless version, which will apply to all pins the object owns.

Using zero's GPIO model means that you cannot accidentally manipulate a GPIO pin that you haven't claimed via a ```Gpio``` object.

If you supply a callback function to the constructor, it will run whenever an input pin assigned to your ```Gpio``` object changes state.

**NOTE:** This callback runs in the context of the PCINT ISR, so be quick, don't block etc. The callback option is provided to allow a very quick response to a pin change, such as might be required when implementing high-speed communications protocols, or you otherwise just need the fastest reaction possible.

If you instead supply a ```Synapse``` to the constructor, it will be signalled whenever an input pin's state changes, and can be detected and handled like any other ```Synapse```-based code.

In both cases, if your ```Gpio``` object owns several input pins, you will need to query the object to find out the current state of those pins, via ```::getInputState()```.

## Example
See ```examples/pinchangedemo``` and ```examples/ledflasher``` for more information. To access the GPIO pins in zero, allocate a ```Gpio``` object on the stack, specifying the pins you want it to have access to. If you cannot gain exclusive access to even a single pin that you want, no pins will be assigned to your new ```Gpio``` object. To check if your allocation has been successful, use the following code...
```
#include "gpio.h"

int myThread
{
    Gpio arduinoUnoLed{ ZERO_PINB5 };

    // check to see if it allocated correctly
    if (arduinoUnoLed) {
        arduinoUnoLed.setAsOutput();
        arduinoUnoLed.switchOff();

        // flash 25 times and then exit
        for ( auto i = 0; i < 50; i++ ) {
            arduinoUnoLed.toggle();
            me.delay( 500 );
        }
    }

    // when arduinoUnoLed goes out of scope here, the
    // pins are deallocated and are now available for
    // another Gpio object to claim

    return 0;
}
```
