# Transmitter
```zero/serial.h```

The ```Transmitter``` is an abstract base class for device drivers implementing a serial interface. Also refer to the documentation for ```Receiver```.

## enable()
Enables the transmitter.
```
    virtual bool enable(
        const Synapse txCompleteSyn
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```txCompleteSyn```|The ```Synapse``` to signal when the transmitter completes a transmission.|

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

Once a transmission is finished, the transmitter will signal the ```txCompleteSyn``` ```Synapse``` supplied in the call to ```enable()```.

## Example
```
#include "thread.h"
#include "usart.h"

int txDemo()
{
    // Signal for learning when we can send again
    SignalField txDoneSig = me.allocateSignal();

    // create a transmitter for hardware USART1
    UsartTx* tx = new UsartTx(1);

    // set comms params and enable the TX
    tx->setCommsParams(9600);
    tx->enable(txDoneSig);

    // main loop
    bool canSend = true;

    while (true) {
        SignalField wokeSigs = 0UL;
        
        if (!canSend) {
            me.wait(txDoneSig);
        
            if (wokeSigs & txDoneSig) {
                // last xmit complete, can send again
                canSend = true;
            }
        }

        if (canSend) {
            tx->transmit((char*) "Beaker is the best puppy ever!\r\n", 32);
            canSend = false;
        }
    }
}
```