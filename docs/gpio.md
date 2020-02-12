# Gpio
```zero/drivers/gpio.h```

The ```Gpio``` class provides protected access to the MCU's GPIO pins.

## Constructor
Constructs a new Gpio object, claiming exclusive access to one or more GPIO pins.

```
    Gpio::Gpio(
        const PinField pins
        )
```
### Parameters
|Param|Description|
|-----|-----------|
|```pins```|A 32-bit bitmap of the pins the object should 'own'.|

### Notes
zero's GPIO model represents up to four (4) GPIO ports in a single ```PinField```, where ```PORTA0``` is bit 0, and ```PORTD7``` is bit 31. This is true even if your target MCU doesn't have a port, for example ```PORTA```, which isn't present on the ATmega328.

The ```pins``` parameter is a bitfield, allowing you to claim multiple pins in a single ```Gpio``` object. These pins can still be manipulated independently of each other by using methods of the ```Gpio``` class, or can be controlled as a group. Most methods, such as ```::setAsOutput()``` have a version that takes a ```PinField``` parameter, which allows you to specify a subset of the pins owned by the object. They will often also have a parameterless version, which will apply to all pins the object owns.

Using zero's GPIO model means that you cannot manipulate a GPIO pin that you haven't claimed via a ```Gpio``` object.

## Example
To access the GPIO pins in zero, allocate a ```Gpio``` object on the stack, specifying the pins you want it to have access to. If you cannot gain exclusive access to even a single pin that you want, no pins will be assigned to your new ```Gpio``` object. To check if your allocation has been successful, use the following code...
```
#include "gpio.h"

int myThread
{
    Gpio arduinoUnoLed( ZERO_PINB5 );

    // check to see if it allocated correctly
    if (arduinoUnoLed) {
        arduinoUnoLed.setAsOutput();
        arduinoUnoLed.switchOff();

        // flash 25 times and then exit
        for (auto i = 0; i < 50; i++ ) {
            arduinoUnoLed.toggle();
            me.wait( 0UL, 500 );
        }
    }

    // when arduinoUnoLed goes out of scope here, the
    // pins are 'deallocated' and are now available for
    // another Gpio object to claim

    return 0;
}
```
