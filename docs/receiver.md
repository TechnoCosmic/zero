# Receiver
```zero/serial.h```

The ```Receiver``` is an abstract base class for device drivers implementing a serial interface. Also refer to the documentation for ```Transmitter```.

## enable()
Enables the receiver.
```
    virtual bool enable(
        const uint16_t bufferSize,
        const Synapse rxSyn,
        const Synapse ovfSyn
        )
```
### Parameters
|Param|Description|
|-----|-----------|
|```bufferSize```|The size of the RX buffer, in bytes.|
|```rxSyn```|A ```Synapse``` that the receiver will signal when data is available.|
|```rxOvfSyn```|A ```Synapse``` that the receiver will signal when the receive buffer overflows.|

## disable()
Disables the receiver, releases any resources used, and prevents further data reception.
```
    virtual void disable()
```

# getCurrentBuffer()
Returns a buffer that contains the currently unprocessed received data.
```
    virtual uint8_t* getCurrentBuffer(
        uint16_t& numBytes
        )
```
|Param|Description|
|-----|-----------|
|```numBytes```|A reference to a ```uint16_t``` used to tell the caller how many bytes are in the returned buffer.|
