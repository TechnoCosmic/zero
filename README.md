# zero

zero is a pre-emptive multi-tasking kernel/OS for AVR architecture MCUs, proudly designed and developed in Australia.

![zero CLI](http://www.tcri.com.au/github/zero_cli.png)

Main features include...

- [x] Fully pre-emptive scheduler
- [x] Dynamic thread creation and `join()` (asynchronous function calls)
- [x] Dynamic memory allocation/reallocation/deallocation
- [x] Pipes for IPC and peripheral communications
- [x] Extensible interactive command line interface for debugging
- [x] Interrupt-driven USART class

zero is very much an active project. Some of the more impactful features planned include...

- [ ] Public 'properties' for easy on-the-fly application configuration
- [ ] Interrupt-driven peripheral classes for I²C, SPI, ADC
- [ ] FAT32 file system support on SD cards

## Licensing

zero does **NOT** use *any* third-party library, or code from any other project. It is entirely self-contained in terms of authorship, dependencies, and licensing, and is released under the MIT License. Please see LICENSE.txt for details.

## Documentation

This file is currently the sole documentation for zero. It is very 'alpha' - expect it to be updated as features evolve.

## Contact Information

### Techno Cosmic Research Institute
- Dirk Mahoney - dirk@tcri.com.au

### Catchpole Robotics
- Christian Catchpole - christian@catchpole.net

## Compatibility

zero is being developed with the ATmega1284 and ATmega328 as it's reference MCUs. However, it is intended to be compatible with other ATmega architecture MCUs. zero may even work on some ATtiny MCUs, if you turn the appropriate knobs and pull the appropriate levers to get everything to fit. Those knobs and levers are found in `zero_config.h`.

The main requirements for zero to function are one 8-bit timer and SRAM enough for some tiny overhead for the kernel itself (back down to less than 100 bytes currently) and your program. You won't necessarily get a fully functioning interactive CLI over USART, and 14 threads calculating the various meanings of life if your MCU is on the lower-end of the AVR architecture spectrum, of course.

You'll find easy access to the MCU settings at the beginning of the `makefile`.

## Very Quick How-To

### Step 1: Declare a Thread

There is no single entry point in zero. Instead, declare a new thread...

```
#include "thread.h"

using namespace zero;


thread(name, stackBytes, {
    // Your awesome code goes here

    return 0;
});
```
`name` is the name for your thread global, as well as the string name for it that the thread() macro will store in Flash memory. `stackBytes` is the number of bytes your thread will need for it's own private stack. The stacks for all Threads are dynamically allocated at runtime.

You can declare as many threads as you like, though your MCU may protest if you go too hard at it. It comes down to how much SRAM each thread needs (both stack and heap), as well as what they're doing.

Just don't include a `main()` function and expect it to work - zero has stolen `main()` for it's own initialization purposes.

### Step 2: Build

Build your source code and link it with the rest of the zero kernel, upload it to your MCU, and you're done.

## Pipes and IPC

Pipes in zero are simple FIFO byte buffers, with support for blocking read/write, and filters. They are used for simple serial buffers for the USART, as well as inter-process communications.

Unlike a lot of other multitasking kernels, and in an effort to keep kernel overhead to a minimum, zero does not automatically imbue a thread with it's own message pipe for IPC/system messages. Instead, you are free to declare only the Pipes you need. This saves precious SRAM as a lot of threads simply don't need IPC.

Here's a simple example...

```
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "thread.h"
#include "pipe.h"

using namespace zero;


// most strings that zero uses are expected to be in Flash
// memory because we like to use our SRAM for *real* data

const char PROGMEM ipcPipeName[] = "ipcPipe";


// declare a thread to send some data to the other thread

thread(sender, 128, {
    // create a 256-byte Pipe named "ipcPipe"
    Pipe ipcPipe(ipcPipeName, 256);

    while (true) {
        // the << operator will block if the Pipe becomes full
        ipcPipe << "This is a test";

        // wait a little bit before sending it again
        _delay_ms(1000);
    }

    return 0;
});


// this thread will receive data from the first thread via a Pipe

thread(receiver, 128, {
    Pipe* ipcPipe = 0UL;

    // find the Pipe that the other thread created

    while (!ipcPipe) {
        ipcPipe = Pipe::find("ipcPipe");
    }

    while (true) {
        uint8_t receivedByte = 0;

        // the boolean argument here means that the
        // call is allowed to block if the Pipe is empty

        if (ipcPipe->read(&receivedByte, true)) {
            // do something with the information
        }
    }

    return 0;
});
```
## Using the USART

The USART driver in zero is completely ISR-driven, meaning that there's no busy-polling when transmitting or receiving data, so performance is as fast as it can possibly be. This is made possible by the Pipes used by the USART driver.

Outgoing data is placed into the transmit Pipe by your program, and will be sent by the AVR's on-board hardware USART peripheral via ISRs while all your threads continue to run. Reception is handled similarly, with the AVR's USART hardware generating an interrupt that places the new data into the receive Pipe. Any threads that were blocked waiting for data are then woken up.

Firing up the USART in zero is as follows...

```
#include "thread.h"
#include "pipe.h"
#include "usart.h"

using namespace zero;

thread(simpleUsartExample, 128, {

    // The first thing we do is create two Pipes
    // to use. One as the receive buffer, and
    // the other for transmission. These two
    // will be created without names for
    // simplicity.

    // NOTE: If we don't want to receive any data,
    // then we don't have to create a Pipe for it.
    // We can tell the USART to fire up only the
    // transmitter, leaving the RX pin for other
    // uses. The same is true in reverse, if you
    // only want to receive data.

    // NOTE: Even though the Pipe objects themselves
    // are local, the 128 byte buffers they create
    // are dynamically allocated on the heap.

    Pipe rx(0UL, 128);
    Pipe tx(0UL, 128);


    // Now create a USART object and supply
    // those two Pipes to the constructor.
    // The first parameter is the baud. Pass in a
    // null pointer for either of the two buffers
    // if you only want RX or TX but not both.

    Usart serial(9600, &rx, &tx);


    // now send stuff, or receive it, or both!

    while (true) {
        tx << "Hello, World!\r\n";
    }

    return 0;
});
```
To receive data from the USART, just read from the receive Pipe you created.

## Memory Management

zero provides a simple page-based dynamic memory allocator. It does not (and because it's for AVR, cannot) provide memory protection services.

After accounting for zero page memory and room for your globals at the front of SRAM, and some kernel stack space at the end of it, the remainder is broken into equal sized 64-byte pages. This page size can be adjusted in `zero_config.h`.

When your program calls `memory::allocate()`, zero searches a very small bit array that keeps track of which of these pages are available, and which are currently in use. If zero finds the number of continguous bytes you asked for, it returns you the address of the start of that block, similar to `malloc()`.

When you're done using that memory, your program is expected to call `memory::deallocate()` to make it available again. Unlike some other memory management systems, zero does not track what pages *each thread* currently owns. This means that when you call `memory::deallocate()`, you must also tell zero how much SRAM to make available again.

There is also a `memory::reallocate()` function that behaves similarly to POSIX `realloc()` insofar as it will find a new chunk of memory of a new size, copy whatever fits from the old chunk to the new chunk, and then free the old chunk. There are plans afoot to make the reallocation algorithm smarter and faster in certain circumstances.

```
uint8_t* buffer;
uint16_t allocated = 0UL;

buffer = memory::allocate(114, &allocated, memory::AllocationSearchDirection::BottomUp);

// do something with the buffer

myClass* myObject = (myClass*) buffer;

myObject->init();

// ...
```
So what's happening there is the program wants no less than 114 bytes of memory. It wants the allocator to start looking for this memory from the low end of SRAM and working up towards all the thread stacks at the far end. It has also provided a pointer to a `uint16_t` where the allocator should put the number of bytes it actually gave the program.

To clarify this a little... zero allocates memory in whole pages, so unless your program asks for an even multiple of the current page size, the caller is going to gain access to a little more memory than it asked for. In some cases, knowing that you have access to a little more than you asked for can be useful, such as in variable length buffers.

The Pipe class does this. If you specify a buffer size in the Pipe's constructor that isn't a multiple of the page size, the Pipe will still make full use of all memory that the allocator gives it. There is also an option for the Pipe to be strict about using the size you gave it, in case your program requires that, by supplying a third parameter to the constructor. When this third parameter, `strictSize`, is `true` the memory allocator will still provide the Pipe will an integer number of pages (that never changes), it's just that the Pipe won't use any extra bytes beyond what it asked for.

If you don't want to know how much you were given, supply a null for the `allocated` parameter.

**NOTE**: zero's memory system isn't a drop-in replacement for anything at the AVR-libc level or the compiler level, so nothing is changed with regards to using `new`. Because of this, don't expect any constructors to work on any object you use with the allocator. Instead, bundle your construction code into a new function and go from there. Stack-based objects will have their constructors called, as usual, and they can call the central `init()` that you create for initializing new objects in allocated memory.

## Advanced Threading - Dynamic Threads/Asynchronous Function Calls

Typically threads are declared at the global scope using the macro demonstrated at the start. These threads start when the scheduler starts, and they aren't really expected to terminate, though they can.

Sometimes you will want to spin up a thread to do some work asynchronously, and pick up the results it generates later. zero provides a mechanism for this.

```
void demo() {

    // Create a new Thread, but it doesn't start running immediately

    Thread* asyncDemo = Thread::create(128, [](){
        // async code

        // do something here that needs to
        // run while you do other things

        // ...

        return 0;
    });

    // Start it running. The `true` parameter here means
    // that when the asyncDemo thread terminates, zero
    // shouldn't clean it up entirely until someone calls
    // join() on it. If you're happy for your dynamic
    // thread to fully disappear and not be able to
    // synchronize with it's termination, use `false` here.

    // NOTE: If you supply `false`, then you must act as if the
    // asyncDemo Thread object is immediately an invalid
    // reference, as you do not know when it has terminated and
    // been automatically cleaned up.

    asyncDemo->run(true);

    // do the other stuff you wanted to do
    // while the asyncDemo thread is running

    // ...

    // Join is a function that will block until asyncDemo
    // terminates. Returns immediately if asyncDemo has
    // already terminated. May or may not crash spectacularly
    // if you supplied `false` to run(), above.

    int returnCode = asyncDemo->join();

    // do other things

    // ...
}
```
If you allow a dynamic thread to be `join()ed` by passing `true` to `run()`, that thread's return code is passed out as the return code from `join()`. This is a simple form of message passing from the dynamic thread back to the main logic. There are many other ways, such as using Pipes as IPC as shown earlier or sharing access to memory, perhaps dynamically allocated.

## Advanced Threading - Atomic Blocks

If you're familiar with the `ATOMIC_BLOCK` macros in `util/atomic.h`, then you'll be immediately comfortable with the zero equivalents (same, but prefixed with `ZERO_`). In zero's case, it doesn't disable hardware interrupts entirely, as `ATOMIC_BLOCK` does, but instead suspends only the kernel's scheduler, so that no thread context swap shenanigans can occur.

```
#include "thread.h"

using namespace zero;

thread(myThread, 128, {
    // do normal stuff here

    // ...

    // this next bit is the critical section
    ZERO_ATOMIC_BLOCK(ZERO_ATOMIC_RESTORESTATE) {
        // do atomic stuff here

        // ...

        // NOTE: Don't call anything that may result
        // in your thread becoming blocked while in
        // a critical section. That's just S.O.P. in
        // a multithreaded environment

        // ...
    }

    // do normal stuff here

    // ...

    return 0;
});
```
zero's atomic macros support both `ZERO_ATOMIC_RESTORESTATE` which re-enables context switching only if it was enabled when the atomic block started, and `ZERO_ATOMIC_FORCEON` which will force context switching back on at the end of the block, regardless of whether it was enabled upon entry.

If you're doing any low level sort of stuff that requires thread-level atomicity, consider if you need to fully disable ISRs via traditional `ATOMIC_BLOCK`, or whether you merely need to disable context switching via the `ZERO_` equivalents.

## Command Line Interface - CLI

zero has an optional command line interface available on MCUs that have a hardware USART. Fire up your favourite VT100 terminal emulator and connect over serial to your MCU. Communications parameters can be found in `zero_config.h`.

A feature of zero is that the CLI is easily extended with custom commands. **These commands are run in the CLI thread's context - no new thread is spawned for each command issued.** A new CLI command can be created as follows...

```
#include "cli.h"

using namespace zero;

clicommand(mycommand, (TextPipe* rx, TextPipe* tx, int argc, char* argv[]) {
    *tx << "Hello, World!\r\n";

    return 0;
});
```

zero will tokenize the command line for you, and hand you the Pipes attached to the USART that the CLI is using. A `TextPipe` is a derived class of `Pipe`, and provides for formatting of strings and numbers, in a similar way to regular `iostream`. zero also provides its own `iomanip` that can be used with a `TextPipe`.

**NOTE:** If your custom CLI commands use significant stack space, then be sure to adjust `CLI_STACK_BYTES` to account for this.

The intention behind the extensible CLI is so that you have a ready-made infrastructure for adding control and debugging features specific to your program's needs.

## Troubleshooting

### Gremlins

The very first thing on troubleshooting - if it looks weird and unexplainable, perhaps even supernatural, then check you've given EVERYTHING plenty of memory - stacks, pipes, allowances for globals and so forth (all found in `zero_config.h`). A great portion of the time, code that used to work and now suddenly doesn't (especially code you'd swear up and down was unrelated) is usually the result of too small a stack, or not enough allowance given for global variables at the start of SRAM space (`GLOBALS_BYTES` in the zero config header).

### Garbage with text on USART with CLI

If you're not using a VT100 terminal emulator to connect to your AVR's newly acquired CLI, then you're probably seeing a bunch of VT100 escape codes that look like rubbish, mixed in with regular text. In `zero_config.h`, you can comment out the `#define CLI_VT100` line to fix this. You won't get all the pretty colours though.

### Garbage without text on USART

Check the `zero_config.h` file to make sure you've got the `CLI_BAUD` set to match the rate in your serial monitor or terminal emulator. You also need 8 data bits, 1 stop bit, no parity bits.
