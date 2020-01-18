# Transmitter
```zero/drivers/serial.h```

The ```Transmitter``` is an abstract base class for device drivers implementing a serial interface. Also refer to the documentation for ```Receiver```.

## enable()
Enables the transmitter.
```
    virtual bool enable(
        Synapse txReadySyn
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```txReadySyn```|The ```Synapse``` to signal when the transmitter is ready to send a new transmission.|

### Notes
The hardware USART and software serial drivers in zero will signal ```txReadySyn``` immediately upon calling ```enable()```.

## disable()
Disables the transmitter, releases any resources used, and prevents further data transmission.
```
    virtual void disable()
```
### Notes
Disabling a transmitter may or may not corrupt or interrupt any transmission in progress at the time, depending on the actual implementation. To avoid this, wait until the previous transmission has completed before disabling the transmitter.

Disabling a transmitter may involve powering down on-board (or external) peripherals, changing the states of GPIO pins, and so on.

## transmit()
Sends information asynchronously.
```
    virtual bool transmit(
        const void* data,
        const uint16_t numBytes
        )
```
|Param|Description|
|-----|-----------|
|```data```|A pointer to the information to transmit.|
|```numBytes```|The number of bytes to send.|
### Notes
A ```Transmitter``` is not required to copy the data you supply here before transmitting it. It is up to your code to ensure the supplied buffer stays current while the transmission is in progress.

Once a transmission is finished, the transmitter will signal the ```txReadySyn``` ```Synapse``` supplied in the call to ```enable()```.

## Example
```
#include "core/thread.h"
#include "drivers/suart.h"

int txDemo()
{
    // Signal for learning when we can send again
    SignalField txReadySig = me.allocateSignal();

    // create a transmitter for software serial TX
    SuartTx* tx = new SuartTx();

    // set comms params - 9600bps on PORTA, PIN0
    tx->setCommsParams(9600, &DDRA, &PORTA, 0);

    // enable the transmitter
    tx->enable(txReadySig);

    // main loop
    while (true) {
        SignalField wokeSigs = me.wait(txReadySig);
    
        if (wokeSigs & txReadySig) {
            // last xmit complete, can send again
            tx->transmit((char*) "Beaker is the best puppy ever!\r\n", 32);
        }
    }
}
```