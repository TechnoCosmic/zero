# SRAM
```zero/drivers/sram.h```

The ```SpiMemory``` class provides high-speed background memory transfers to supported external SPI memory ICs.

Any SPI memory IC that is protocol-compatible with the Atmel/Microchip 23LCxxxx/25LCxxxx can be used. These include SRAM, NVSRAM, EERAM, and EEPROM ICs.

## Constructor
```
        SpiMemory(
            const uint32_t capacityBytes,
            const Gpio& chipSelect,
            Synapse readySyn
            )
```
|Param|Description|
|-----|-----------|
|```capacityBytes```|The total number of bytes the chip holds.|
|```chipSelect```|A reference to a ```Gpio``` object that represents your chosen CS pin.|
|```readySyn```|A ```Synapse``` to signal when the driver is ready to accept another read/write request.|

### Notes
As they all share access to the MCU's single hardware SPI peripheral, only one ```SpiMemory``` object can be using the SPI bus at any given time. This can be assured by waiting until the last ```SpiMemory``` object that used the bus has signalled it's ready ```Synapse``` before you begin a new ```read()``` or ```write()``` on the same or another ```SpiMemory``` object.

All ```SpiMemory``` objects *should* be handed the same ```Synapse``` as each other, but this is not required.

## read()
Begins an asychronous transfer of data from an external SPI memory chip, to the internal SRAM of the MCU.

```
    void read(
        void* dest,
        const uint32_t srcAddr,
        const uint32_t numBytes
        )
```
|Param|Description|
|-----|-----------|
|```dest```|A pointer to an address in local MCU SRAM where the incoming data from the SPI memory chip should be copied.|
|```srcAddr```|An address specifying the start of the source data in external memory.|
|```numBytes```|The number of bytes to transfer.|

### Notes
Once the transfer has begun, control wil return to your code. Once the transfer has finished, the ```SpiMemory``` object will signal the ```Synapse``` you supplied in the constructor.

## write()
Begins an asychronous transfer of data from local MCU SRAM, to the external memory chip.

```
    void write(
        const void* src,
        const uint32_t destAddress,
        const uint32_t numBytes
        )
```
|Param|Description|
|-----|-----------|
|```src```|A pointer to an address in local MCU SRAM from where the data destined for the SPI memory chip should be copied.|
|```destAddr```|An address specifying the destination of the data, in external memory.|
|```numBytes```|The number of bytes to transfer.|

### Notes
Once the transfer has begun, control wil return to your code. Once the transfer has finished, the ```SpiMemory``` object will signal the ```Synapse``` you supplied in the constructor.

## Example
```
#include "thread.h"
#include "sram.h"

int spiMemoryDemo
{
    Gpio csPin( ZERO_PINB4 );                           // CS for the SPI memory chip
    Synapse sramReadySig;                               // to learn when the xfer is done
    SpiMemory extSram(
        131072ULL,                                      // I'm using a 23LCV1024 (1Mbit)
        csPin,
        sramReadySig
    );

    // wait for the SRAM to be ready (immediately after init, thankfully!)
    sramReadySig.wait();

    // send it some data
    extSram.write( "Dogs are the best!\000", 19 );

    // do other things while that's happening
    // ...

    // allocate somewhere to put the incoming data
    auto buffer = (char*) memory::allocate(19);

    // wait for it to be done, then read it back
    sramReadySig.wait();

    // we *should* check that it allocated okay, but this is demo code
    extSram.read( buffer, 19 );

    // wait for that to come back in...
    sramReadySig.wait();

    // it's here, do something with it...
    debug::print( buffer );

    // clean up
    memory:free( buffer, 19 );
    buffer = nullptr;
}
```
