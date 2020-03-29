# zero AVR kernel
zero is a tiny pre-emptive multitasking kernel for AVR microcontrollers. Specifically, zero is built with the ATmega328, ATmega644, and ATmega1284 in mind, though many others will work with the appropriate tweaks.

## Features
- Small footprint - core multitasking kernel and memory manager (sans communications drivers) is 3.2KB, and uses 64 bytes of SRAM
- O(1) scheduler
- Pool threads
- Dynamic memory allocation
- Multi-participant Watchdog
- Pipes for IPC
- Protected GPIO access
- Asynchronous external SPI SRAM driver
- Asychronouus ADC
- Hardware and software UART
- [Documentation](http://zero.tcri.com.au)

## Under Construction
- Location Services
- ESP WiFi Driver

## Documentation
This document serves as an introduction to and conceptual description of zero. For API documentation, please go to [the zero website](http://zero.tcri.com.au). If you have Doxygen and Graphviz installed, you can generate your own documentation locally - just look for ```DOXYGEN``` in the ```makefile``` and set it to ```1``` and it will be re-built when you compile.

## Threading Model
zero's threading model is a simple one...

 - No explicit priorities - round-robin time slicing
 - Active/Expired thread ready lists for O(1) selection of the next thread to run
 - Idle thread is implied lowest-priority, running only when no other thread wants to run
 - Signals implement the blocking system - a Thread that is ```wait()```ing is not in either ready list and will not run
 - System Thread pool for fast thread spin-up

 The ```Thread``` class is very data-lean (27 bytes per ```Thread```). ```SREG``` is stored on the Thread's stack (as is ```RAMPZ``` on those MCUs that use it).

 ## Scheduler
 zero's scheduler maintains two (2) doubly-linked lists of ```Thread``` objects - Active, and Expired, which enables zero to implement context-switching in O(1) time.

If a Thread uses all of it's quantum, it will be moved to the Expired list when it is pre-empted. Once all Threads in the Active list have expired, the Active list will be empty. The scheduler simply swaps lists and starts executing from the head of the old Expired list (which is now considered the Active list) and the process repeats. If at any stage there are no Threads on either the Active or the Expired lists, the idle thread will be selected.

Threads that yield control of the MCU voluntarily by way of calling ```wait()``` are taken out of both lists, and cannot execute again until another Thread or device driver calls ```signal()``` on that Thread (with one or more signals that the Thread is waiting for).

See ```docs/thread.md``` for API reference.

## Dynamic Memory Allocation
zero implements a simple page-based memory manager, with overrides for ```new``` and ```delete```. See ```docs/memory.md``` for API reference.

## Hardware and Software USART Drivers
zero's serial I/O model implemented by transmitters (```UsartTx``` and ```SuartTx```) and receivers (```UsartRx```). See ```docs/transmitter.md``` and ```docs/receiver.md``` for API reference.

## GPIO Subsystem

zero implements a protected GPIO model, ensuring code only accesses GPIO pins to which it has access. See ```docs/gpio.md``` for API reference.

## External SPI Memory
zero supports the use of external SPI memory ICs, namely those that are protocol-compatible with Atmel/Microchip's 23LCxxxx and 25LCxxxx memory chips. You can also use multiple of these devices on the same SPI bus, each with the same or different capacities.

Using a very straightforward read/write model, you begin an asynchronous transfer between on-board SRAM and external memory, and ```wait()``` on a signal to learn when it's complete. See ```docs/sram.md``` for API reference.
