# UsartRx
```zero/drivers/usart.h```

## enable()
Enables the USART receiver.
```
    bool enable(
        const uint16_t bufferSize,
        Synapse& rxSyn,
        Synapse* ovfSyn
        )
```
### Parameters
|Param|Description|
|-----|-----------|
|```bufferSize```|The size of the receive buffer, in bytes.|
|```rxSyn```|A ```Synapse``` that the receiver will signal when data is available.|
|```rxOvfSyn```|A ```Synapse``` that the receiver will signal when the receive buffer overflows.|

### Notes
The serial device drivers in zero use the ```DoubleBuffer``` class to implement their receive buffers. For this reason, the effective buffer size at any given moment is only half of ```bufferSize```, since ```bufferSize``` specifies the total memory used by both buffers.

## disable()
Disables the USART, releases any resources used, and prevents further data reception.
```
    void disable()
```

## getCurrentBuffer()
Returns a buffer that contains the currently unprocessed received data.
```
    uint8_t* getCurrentBuffer(
        uint16_t& numBytes
        )
```
|Param|Description|
|-----|-----------|
|```numBytes```|A reference to a ```uint16_t``` used to tell the caller how many bytes are in the returned buffer.|

## flush()
Discards the current input buffer.

## Example
```
#include "thread.h"
#include "synapse.h"
#include "usart.h"

int rxDemo()
{
    // Synapses for communicating with the receiver
    Synapse rxSyn;
    Synapse rxOvfSyn;
    
    // create a receiver for hardware USART1
    UsartRx rx(1);

    // set the comms parameters and enable the RX
    rx.setCommsParams(9600);
    rx.enable(128, rxSig, &rxOvfSig);

    // main loop
    while (true) {
        auto wokeSigs = me.wait(rxSyn | rxOvfSyn);

        if (wokeSigs & rxSyn) {
            // data received, get the RX buffer
            uint16_t numBytesRecd;
            
            while (uint8_t* rxBuffer = rx.getCurrentBuffer(numBytesRecd)) {
                // do something with the received data
                processRxData(rxBuffer, numBytesRecd);
            }
        }

        if (wokeSigs & rxOvfSyn) {
            // buffer overflow - need a bigger buffer
        }
    }
}
```
