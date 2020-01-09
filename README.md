# zero AVR kernel
zero is a tiny pre-emptive multitasking kernel for AVR microcontrollers. Specifically, zero is built with the ATmega328, ATmega644, and ATmega1284 in mind, though many others will work with the appropriate tweaks.

## Features
- Small footprint - a little under 7KB binary, and 192 bytes of SRAM
- O(1) scheduler
- Dynamic memory allocation
- Drivers for on-chip hardware USART peripherals
- Driver for software interrupt-driven UART
- Asynchronous SPI memory driver for supported Atmel ICs

## Documentation
This document serves as an introduction to and conceptual description of zero. For API documentation, please see the ```docs``` folder.

## Threading Model
zero's threading model is a simple one...

 - No explicit priorities - round-robin time slicing
 - Active/Expired thread ready lists for O(1) selection of the next thread to run
 - Idle thread is implied lowest-priority, running only when no other thread wants to run
 - Signals implement the blocking system - a Thread that is ```wait()```ing is not in either ready list and will not run
 
 The ```Thread``` class is very data-lean (17 bytes per ```Thread```). ```SREG``` is stored on the Thread's stack (as is ```RAMPZ``` on those MCUs that use it).

 ## Scheduler
 zero's scheduler maintains two (2) doubly-linked lists of ```Thread``` objects - Active, and Expired, which enables zero to implement context-switching in O(1) time.

If a Thread uses all of it's quantum, it will be moved to the Expired list when it is pre-empted. Once all Threads in the Active list have expired, the Active list will be empty. The scheduler simply swaps lists and starts executing from the head of the old Expired list (which is now considered the Active list) and the process repeats. If at any stage there are no Threads on either the Active or the Expired lists, the idle thread will be selected.

Threads that yield control of the MCU voluntarily by way of calling ```wait()``` are taken out of both lists, and cannot execute again until another Thread or device driver calls ```signal()``` on that Thread (with one or more signals that the Thread is waiting for).

See ```docs/thread.md``` for API reference.

## Dynamic Memory Allocation
zero implements a simple page-based memory manager, with overrides for ```new``` and ```delete```. See ```docs/memory.md``` for API reference.

## Hardware USART Drivers
zero's serial I/O model implemented by Transmitters and Receivers.

### Transmitting Data
After creating an instance of a ```Transmitter```-derived class, such as ```UsartTx```, you set up the communications parameters (specific to the type of transmitter), and enable it. In the case of the AVR on-board USART devices, the only parameter needed is the baud rate. If you want to use zero's software serial transmitter, you will also need to specify the DDR, PORT and pin for the TX line.

To transmit data, call ```transmit()``` on the transmitter. The transmitter will then *asynchronously* transmit the information handed to the ```transmit()``` function. This means that whatever data or buffer you transmit must remain valid and untouched until the transmission is complete, as ***no copy of the supplied information is performed***.

Transmission is complete when the transmitter signals the ```Synapse``` that it was handed when it was ```enable()```'d. A ```Synapse``` is a simple ```Thread```/```SignalField``` pair. See ```docs/synapse.md``` for more information.

#### Example
```
int txDemoThread()
{
    SignalField txReadySig = me.allocateSignal();
    UsartTx* tx = new UsartTx(0);

    tx->setCommsParams(9600);
    tx->enable(txReadySig);

    while (true) {
        SignalField wokeSigs = me.wait(txReadySig);

        if (wokeSigs & txReadySig) {
            tx->transmit("Hello, World!\r\n", 15);
        }
    }

    // clean up goes here, if reqd
}

```

### Receiving Data
To receive data serially in zero, you can create an instance of a ```Receiver```-derived class, such as ```UsartRx```. As with a transmitter, a receiver needs to be told it's communications parameters (such as baud rate), and also needs to be enabled.

When you enable a ```Receiver```, you hand it three (3) pieces of information...

- Buffer size, in bytes
- A ```Synapse``` to be signalled when data is available for your program to process
- A ```Synapse``` to be signalled if/when the receive buffer overflows

To receive data, your Thread should ```wait()``` on the signal it allocated for the "data available" ```Synapse```. When this signal is set, you can ask the receiver for the current receive buffer...
```
int mySerialThread()
{
    // allocate signals for rx interaction
    SignalField rxDataAvailSig = me.allocateSignal();
    SignalField rxOvfSig = me.allocateSignal();

    // allocate a serial receiver on USART0
    UsartRx* rx = new UsartRx(0);

    // order matters - comms params, then enable
    rx->setCommsParams(9600);
    rx->enable(128, rxDataAvailSig, rxOvfSig);

    // main loop
    while (true) {
        // block waiting for data or buffer overflow
        SignalField wokeSigs = me.wait(rxDataAvailSig | rxOvfSig);

        if (wokeSigs & rxDataAvailSig) {
            // data has arrived at the USART0 device!
            uint16_t numBytes;

            while (uint8_t* rxData = rx->getCurrentBuffer(numBytes)) {
                // numBytes now contains the number of bytes
                // actually received in the current buffer
                processRxData(rxData, numBytes);
            }
        }

        if (wokeSigs & rxOvfSig) {
            // RX overflow - we need a bigger buffer!
        }
    }
}
```
### Notes
The receiver uses a double-buffered approach. Because of this, the actual size of the active receive buffer is half that specified in the ```enable()``` call. One half of the buffer is filling with incoming data while your code is processing the data contained in the other half. ```getCurrentBuffer()``` returns a pointer to the data received since the last call to ```getCurrentBuffer()```. ***It does not return of copy of that data***. If there is no data available, ```getCurrentBuffer()``` will return a null pointer, and the supplied reference argument, ```numBytes```, will be set to zero (0).
