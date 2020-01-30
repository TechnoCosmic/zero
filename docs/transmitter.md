# UsartTx/SuartTx
```
zero/drivers/usart.h
zero/drivers/suart.h
```

## enable()
Enables the transmitter.
```
    bool enable(
        Synapse& txReadySyn
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
    void disable()
```
### Notes
Disabling a transmitter may or may not corrupt or interrupt any transmission in progress at the time, depending on the actual implementation. To avoid this, wait until the previous transmission has completed before disabling the transmitter.

Disabling a transmitter may involve powering down on-board (or external) peripherals, changing the states of GPIO pins, and so on.

## transmit()
Sends information asynchronously.
```
    bool transmit(
        const void* data,
        const uint16_t numBytes,
        const bool allowBlock = false
        )
```
|Param|Description|
|-----|-----------|
|```data```|A pointer to the information to transmit.|
|```numBytes```|The number of bytes to send.|
|```allowBlock```|If ```true```, the transmitter will ```wait()``` on the ```txReadySyn``` before sending. Optional, default ```false```.|

### Notes
A transmitter is not required to copy the data you supply here before transmitting it. It is up to your code to ensure the supplied buffer stays current while the transmission is in progress.

Once a transmission is finished, the transmitter will signal the ```txReadySyn``` ```Synapse``` supplied in the call to ```enable()```.

## Example
```
#include "thread.h"
#include "synapse.h"
#include "suart.h"

int txDemo()
{
    // Synapse for learning when we can send again
    Synapse txReadySyn;

    // create a transmitter for software serial TX
    SuartTx tx;

    // set comms params - 9600bps on PORTA, PIN0
    tx.setCommsParams(9600, &DDRA, &PORTA, 0);

    // enable the transmitter
    tx.enable(txReadySyn);

    // main loop
    while (true) {
        tx.transmit((char*) "Beaker is the best puppy ever!\r\n", 32, true);
    }
}
```
