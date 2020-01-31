# zero AVR kernel
zero is a tiny pre-emptive multitasking kernel for AVR microcontrollers. Specifically, zero is built with the ATmega328, ATmega644, and ATmega1284 in mind, though many others will work with the appropriate tweaks.

## Features
- Small footprint - core multitasking kernel and memory manager (sans communications drivers) is 3.2KB, and uses 64 bytes of SRAM
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
 
 The ```Thread``` class is very data-lean (23 bytes per ```Thread```). ```SREG``` is stored on the Thread's stack (as is ```RAMPZ``` on those MCUs that use it).

 ## Scheduler
 zero's scheduler maintains two (2) doubly-linked lists of ```Thread``` objects - Active, and Expired, which enables zero to implement context-switching in O(1) time.

If a Thread uses all of it's quantum, it will be moved to the Expired list when it is pre-empted. Once all Threads in the Active list have expired, the Active list will be empty. The scheduler simply swaps lists and starts executing from the head of the old Expired list (which is now considered the Active list) and the process repeats. If at any stage there are no Threads on either the Active or the Expired lists, the idle thread will be selected.

Threads that yield control of the MCU voluntarily by way of calling ```wait()``` are taken out of both lists, and cannot execute again until another Thread or device driver calls ```signal()``` on that Thread (with one or more signals that the Thread is waiting for).

See ```docs/thread.md``` for API reference.

## Dynamic Memory Allocation
zero implements a simple page-based memory manager, with overrides for ```new``` and ```delete```. See ```docs/memory.md``` for API reference.

## Hardware USART Drivers
zero's serial I/O model implemented by transmitters (```UsartTx``` and ```SuartTx```) and receivers (```UsartRx```). See ```docs/transmitter.md``` and ```docs/receiver.md``` for API reference.

## External SPI Memory
zero supports the use of external SPI memory ICs, namely those that are protocol-compatible with Atmel/Microchip's 23LCxxxx and 25LCxxxx memory chips. You can also use multiple of these devices on the same SPI bus, each with the same or different capacities.

Using a very straightforward read/write model, you begin an asynchronous transfer between on-board SRAM and external memory, and ```wait()``` on a signal to learn when it's complete. See ```docs/sram.md``` for API reference.
