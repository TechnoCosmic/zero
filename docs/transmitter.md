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
A specific implementation of a ```Transmitter``` is not required to copy the data you supply here before transmitting it. It is up to your code to ensure the supplied buffer stays current while the transmission is in progress.

Once a transmission is finished, the transmitter will signal the ```txCompleteSyn``` ```Synapse``` supplied in the call to ```enable()```.
