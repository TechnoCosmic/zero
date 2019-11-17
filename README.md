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

## Latest Changes

Major things in the updates will be listed here. Bug fixes, refactoring, tidy ups are all implied in every update, so they won't be mentioned.

| Date | Ver | Comments |
| ---- | ---:| -------- |
2019-11-1x | 0.7 | 
2019-11-17 | 0.6 | Single line command history added to CLI
2019-11-16 | 0.5 | Idle thread set up code heavily cut. Prep for data structure footprint shrink
2019-11-15 | 0.4 | Overhaul of how SRAM is handled - see the top of the `makefile` for info
2019-11-14 | 0.3 | Initial implementation of `Thread::waitUntil()` for a blocking `delay()`

## Licensing

zero does not use any third-party library, or code from any other project. It is entirely self-contained in terms of authorship, dependencies, and licensing, and is released under the MIT License. Please see LICENSE.txt for details.

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

### startup_sequence()

To start everything off, write some code as follows...

![zero blink](http://www.tcri.com.au/github/zero_blink.png)

The `startup_sequence` function is a required function to kickstart your zero-based program. Do any initial configuration or set up here, including defining any initial Threads. You'll need at least one.

The example shown starts two Threads, each flashing it's own LED on a GPIO pin.

### Step 2: Build

Build your source code and link it with the rest of the zero kernel, upload it to your MCU, and you're done.

### The makefile

The default makefile for zero isn't a simple one, in order to make it easy to configure your build.

The first lines of interest look like this...
```
OUTPUT = monty
AVRDUDE_PART = m1284p
AVRDUDE_CFG = pi
F_CPU = 16000000UL
```
`OUTPUT` specifies the name of your project. For example, [monty](https://github.com/slipperyseal/monty "Monty Stereo SID Synth").

`AVRDUDE_PART` is the MCU part number as avrdude would expect as it's `-p` parameter. Stuff later on in the makefile converts this to the value required for avr-gcc's `-mmcu` parameter.

`AVRDUDE_CFG` is the programming config option that avrdude would use to upload your compiled binary to your MCU, as expected by it's `-c` parameter.

`F_CPU` is the speed, in **Hz** (not MHz) of your target MCU. zero uses this to scale it's Timer and USART settings automatically. You should be able to choose a clock frequency of anywhere between 1MHz and 32MHz and zero will correctly adjust accordingly. Whether your MCU copes with your chosen speed is another story.

With those basic settings out of the way, here are the build targets for the makefile and what they're for...

- `make` - Builds the binaries
- `make clean` - Removes all temporary files and binary outputs
- `make upload` - Builds the binaries and uploads the firmware to your MCU
- `make gettools` - Downloads and installs all the avr-gcc bits and pieces needed to build zero. You probably already have these.

## Pipes and IPC

Pipes in zero are simple FIFO byte buffers, with support for blocking read/write, and filters. They are used for simple serial buffers for the USART, as well as inter-process communications.

Unlike a lot of other multitasking kernels, and in an effort to keep kernel overhead to a minimum, zero does not automatically imbue a thread with it's own message pipe for IPC/system messages. Instead, you are free to declare only the Pipes you need. This saves precious SRAM as a lot of threads simply don't need IPC.

Here's a simple example...

```
#include <avr/pgmspace.h>
#include "thread.h"
#include "pipe.h"

using namespace zero;

// most strings that zero uses are expected to be in Flash
// memory because we like to use our SRAM for *real* data

const char PROGMEM ipcPipeName[] = "ipcPipe";

// declare a thread to send some data to the other thread

int firstThread() {
    // create a 256-byte Pipe named "ipcPipe"
    Pipe ipcPipe(ipcPipeName, 256);

    while (true) {
        // the << operator will block if the Pipe becomes full
        ipcPipe << "This is a test" << endl;

        // wait a little bit before sending it again
        delay(1000);
    }

    return 0;
}


// this thread will receive data from the first thread via a Pipe

int secondThread() {
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
}
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

int usartExampleThread() {

    // The first thing we do is create two Pipes
    // to use. One as the receive buffer, and
    // the other for transmission.

    // NOTE: If we don't want to receive any data,
    // then we don't have to create a Pipe for it.
    // We can tell the USART to fire up only the
    // transmitter, leaving the RX pin for other
    // uses. The same is true in reverse, if you
    // only want to receive data.

    // NOTE: Even though the Pipe objects themselves
    // are local, the 128 byte buffers they create
    // are dynamically allocated on the heap.

    Pipe rx(PSTR("pipe_rx"), 128);
    Pipe tx(PSTR("pipe_tx"), 128);

    // Now create a USART object and supply
    // those two Pipes to the constructor.
    // The first parameter is the baud. Pass in a
    // null pointer for either of the two buffers
    // if you only want RX or TX but not both.

    Usart serial(9600, &rx, &tx);

    // now send stuff, or receive it, or both!

    while (true) {
        tx << "Hello, World!" << endl;
    }

    return 0;
}
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

buffer = memory::allocate(114, &allocated, memory::SearchStrategy::BottomUp);

// do something with the buffer

char* myString = (char*) buffer;


// ...
```
So what's happening there is the program wants no less than 114 bytes of memory. It wants the allocator to start looking for this memory from the low end of SRAM and working up towards all the thread stacks at the far end. It has also provided a pointer to a `uint16_t` where the allocator should put the number of bytes it actually gave the program.

To clarify this a little... zero allocates memory in whole pages, so unless your program asks for an even multiple of the current page size, the caller is going to gain access to a little more memory than it asked for. In some cases, knowing that you have access to a little more than you asked for can be useful, such as in variable length buffers.

The Pipe class does this. If you specify a buffer size in the Pipe's constructor that isn't a multiple of the page size, the Pipe will still make full use of all memory that the allocator gives it. There is also an option for the Pipe to be strict about using the size you gave it, in case your program requires that, by supplying a third parameter to the constructor. When this parameter, `strictSize`, is `true` the memory allocator will still provide the Pipe will an integer number of pages (that never changes), it's just that the Pipe won't use any extra bytes beyond what it asked for.

If you don't want to know how much you were given, supply a null for the `allocated` parameter.

**UPDATE**: There is now support for `new` and `delete` operators to use zero's allocator. There is *NO* support for allocating arrays of things using these operators. This is because the implementation being used is an unofficial extension to the C++ bits and pieces, and array deallocation doesn't seem to be included in on the fun because it's implementation at the compiler-level is probably a little complex. For arrays of things, use `memory::allocate()` as described above. You won't be able to use constructors and destructors, but for arrays that's easily worked around.

```
class Dummy {
    public:
        Dummy(int someInitValue) {
            // ...
            ;
        }

    private:
        // this char array works fine, if you're
        // allocating it in an object at fixed size
        char _myString[128];
        in16_t _myOtherMemberVar;
};

void doAmazingThings() {
    Dummy* d = new Dummy(42);

    // do things
    // ...

    delete d;
    d = 0UL;
}
```
This will work as expected. The `new` operator uses the `BottomUp` strategy when calling into `memory::allocate()`.

## Advanced Threading - Dynamic Threads/Asynchronous Function Calls

Typically threads are declared at the global scope using the macro demonstrated at the start. These threads start when the scheduler starts, and they aren't really expected to terminate, though they can.

Sometimes you will want to spin up a thread to do some work asynchronously, and pick up the results it generates later. zero provides a mechanism for this.

```
void demo() {

    // Create a new Thread, and it will start running ASAP

    Thread* asyncDemo = new Thread(0UL, 0, 0, [](){
        // async code

        // do something here that needs to
        // run while you do other things

        // ...

        return 0;
    }, TLF_READY);

    // do the other stuff you wanted to do
    // while the asyncDemo thread is running

    // ...

    // Join is a function that will block until asyncDemo
    // terminates. Returns immediately if asyncDemo has
    // already terminated. May or may not crash spectacularly
    // if you supplied TLF_AUTO_CLEANUP in the launch flags
    // to the Thread's constructor.

    int returnCode = asyncDemo->join();

    // do other things

    // ...
}
```
If you `join()` a Thread that is NOT flagged for auto-cleanup, that thread's return code is passed out as the return code from `join()`. This is a simple form of message passing from the dynamic thread back to the main logic. There are many other ways, such as using Pipes as IPC as shown earlier or sharing access to memory, perhaps dynamically allocated.

## Advanced Threading - Atomic Blocks

If you're familiar with the `ATOMIC_BLOCK` macros in `util/atomic.h`, then you'll be immediately comfortable with the zero equivalents (same, but prefixed with `ZERO_`). In zero's case, it doesn't disable hardware interrupts entirely, as `ATOMIC_BLOCK` does, but instead suspends only the kernel's scheduler, so that no thread context swap shenanigans can occur.

```
#include "thread.h"

using namespace zero;

void myFunc()
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
}
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
    *tx << "Hello, World!" << endl;

    return 0;
});
```

zero will tokenize the command line for you, and hand you the Pipes attached to the USART that the CLI is using. A `TextPipe` is a derived class of `Pipe`, and provides for formatting of strings and numbers, in a similar way to regular `iostream`. zero also provides its own `iomanip` that can be used with a `TextPipe`.

**NOTE:** If your custom CLI commands use significant stack space, then be sure to adjust `CLI_STACK_BYTES` to account for this.

The intention behind the extensible CLI is so that you have a ready-made infrastructure for adding control and debugging features specific to your program's needs.

### CLI Basic Features

The CLI is very rudimentary. Hitting ESC will clear the currently entered command line, BACKSPACE will erase the previous character entered, pressing ENTER/RETURN will try to execute the current command line. There is currently no cursor key support so command line editing is limited to what we see here.

#### TAB Auto-Completion

Pressing TAB will perform auto-completion on the last word you've typed, by searching through the system objects list (as displayed by the `ls` command) for the first thing it finds that matches.

**NOTE:** This is not a sorted list, so don't expect the first alphabetical match.

### Built-in CLI Commands

zero comes with a suite of CLI commands, which you may or may not find useful, depending on your needs.

Process Status - `ps`

![zero ps](http://www.tcri.com.au/github/zero_ps.png)

Provides a listing of all the current threads, showing their state (running, paused etc) as well as information about the stack (where it is, how much is used), as well as the amount of actual runtime the thread has received so far. If `INSTRUMENTATION` (from `zero_config.h`) is not defined, then `ps` will not show as much information as it normally would, since zero isn't recording as much.

Memory Map - `memmap`

![zero memmap](http://www.tcri.com.au/github/zero_memmap.png)

A colour-coded overview of the SRAM in the MCU, and how it's being used.

Memory Dump - `rram`, `rflash`, `reeprom`

![zero memorydump](http://www.tcri.com.au/github/zero_memorydump.png)

These commands provide a hexadecimal display of memory, 256 bytes at a time. They each take a single parameter, which is the 4-digit (hex) address of the memory to show. Two of these commands have companion 'write' versions, `wram` and `weeprom`. They take a third parameter which is the string you'd like written to memory...

`weeprom 0100 "My dog is the best boy ever!"`

Thread Control - `play`, `pause`

These two commands do as they suggest. They take multiple parameters, being the names of the threads to affect (as shown by the `ps` command), and are case-sensitive.

Clear Screen - `clear`

I like a tidy screen, and `clear` does that. :)

## Troubleshooting

### Gremlins

The very first thing on troubleshooting - if it looks weird and unexplainable, perhaps even supernatural, then check you've given EVERYTHING plenty of memory - stacks, pipes, allowances for the dynamic allocator and so forth (all found in `zero_config.h`). A great portion of the time, code that used to work and now suddenly doesn't (especially code you'd swear up and down was unrelated) is usually the result of too small a stack, or something along those lines.

### Garbage with text on USART with CLI

If you're not using a VT100 terminal emulator to connect to your AVR's newly acquired CLI, then you're probably seeing a bunch of VT100 escape codes that look like rubbish, mixed in with regular text. In `zero_config.h`, you can comment out the `#define CLI_VT100` line to fix this. You won't get all the pretty colours though.

### Garbage without text on USART

Check the `zero_config.h` file to make sure you've got the `CLI_BAUD` set to match the rate in your serial monitor or terminal emulator. You also need 8 data bits, 1 stop bit, no parity bits.


## Under the Hood - Scheduler

zero has a properly pre-emptive scheduler, providing full context save and restore. A forced context switch is generally triggered by an ISR attached to one of your MCU's timer peripherals. By default, this is 8-bit Timer2 on the reference ATmega MCUs. A manual context switch is triggered by a call to `Thread::block()`, but this is usually done by lower-level components such as peripheral drivers or Pipes.

### Thread Control Block Structure

A `Thread` in zero is a handful of bytes of information regarding it's name (a pointer into Flash memory, not precious SRAM), it's entry point, it's current state, launch flags, stack information, and so on. Details can be found in `thread.h`. One main piece of information is how much of it's quantum is left before it will be booted out in favour of another Thread, `_remainingTicks`. When this reaches 0, the Thread will be pre-empted. One tick is currently one (1) millisecond. By default, zero Threads have a 15ms quantum. You can change the default in `zero_config.h`. You can also give each Thread it's own timeslice setting, by supplying a non-zero value to the `quantumOverride` argument.

### Scheduler Operation

The scheduler maintains a solitary doubly-linked list of Threads, be they ready, running, blocked, or otherwise. This will change to a more optimal mechanism in the not-too-distant future. Whether triggered by the ISR, or via `::block()`, a context switch is started by a call to `yield()` or `yield_to()` (implemented in `thread.cpp`). A context switch consists of three main steps...

- Preserve the current contents of all registers onto the current Thread's stack. This is implemented by an inline assembly language macro found in `thread_macros.h`. zero also preserves the contents of the SP, SREG, and RAMPZ (if applicable). Some kernels save SREG on the Thread's stack, others in the Thread object. zero stores it in the Thread object. This is done in `saveCurrentContext()`.

**NOTE:** As part of this step, the SP is temporarily updated to the original SP that was in place when the main initialisation of the kernel occurred (just prior to starting the scheduler). This is considered the kernel stack, and is the stack used by the next step. This is so that individual Thread stacks don't need to account for any extra kernel overhead.

- Choose the next Thread to resume. This is done via a call to `selectNextThread()`. It simply steps through the Thread list, stopping when it comes to a Thread that is ready to run, looping back to the head of the list if necessary.

- Restore the context of the newly chosen Thread. This is done in `restoreContext()`.

At this point, the yield finishes with a `reti` instruction, which pops the new Thread's PC off the Thread's own stack, re-enables ISRs, and the chosen Thread's execution resumes immediately.

## Under the Hood - Memory Allocator

The dynamic memory allocator in zero is fairly simple. Using some constants in `zero_config,h` to figure out how much of the MCU's SRAM you want to give to the allocator to hand out, zero conceptually divides this into pages of `PAGE_BYTES`. These pages are tracked using a small bitmapped array. Although even simpler methods exist to track free space using the free space itself (essentially costing no memory to track), such a method wouldn't translate well into slower access external memories, such as SPI SRAM/EEPROM. The finding of free space and the marking of it as allocated/free would be forced through that serial access mechanism, slowing the allocation/deallocation process down. This is even more true if the communications mechanism, such as SPI, has many devices on the bus, forcing the allocator to wait it's turn to even begin the allocation process.

**NOTE:** Despite the underlying mechanism for the allocator being page-based, all API calls for the allocator are in bytes, as you'd hope. The allocator internally translates this into the correct number of pages and back again, as required.

### Allocation

Due to this approach, `memory::allocate()` looks a little different to typical `malloc()` style functions. zero's incarnation takes two parameters beyond the usual size of the memory block requested.

`uint8_t* memory::allocate(const uint16_t numBytes, uint16_t* allocatedBytes, const SearchStrategy strategy)`

`uint16_t numBytes`

The first parameter is the easiest one - how many bytes of memory do you need?

`uint16_t* allocated`

Because memory is allocated in pages, every successful allocation will be for a chunk of memory that is an even multiple of `PAGE_BYTES`. So when you ask for 24 bytes when `PAGE_BYTES` is 32, you will given 32 bytes, and you have access to all 32 of those bytes, if you want them. `memory::allocate()` can optionally take a pointer to a `uint16_t` where it will tell you how many bytes you *actually* received from the allocator, if your program could benefit from knowing.

zero's built-in `Pipe` class does this. Because a pipe is a variable length buffer, almost by definition, then if it gets more bytes than it asked for, it will use them.

`SearchStrategy strategy`

The next extra paramater that `memory::allocate()` can accept is an enum that directs the allocator to use a particular search alogithm when scanning the bitmap for available pages.

The possibilities are...

`BottomUp`

In typical memory allocators, the search for free space usually begins at regions towards the lower end of address space, proceeeding upwards. The `BottomUp` strategy is this same method, beginning it's search at page 0, proceeding upwards into higher pages.

**NOTE:** Page 0 is NOT page zero in the MCU. Page 0, as far the PageManager is concerned, is the first page in whatever physical area of SRAM the compiler assigned to the `uint8_t` array that is used. This array is called `_memoryArea` found in `memory.cpp`. It is this array that is the reason that GCC will show a huge amount of data used.

`TopDown`

The exact opposite of `BottomUp` in name as well as function. It starts at the last page and searches backwards through the pages to find a contiguous chunk of pages of the required size.

Why have these two strategies? Speed.

Being able to allocate all your longer-lived data structures at one end of the dynamic space, and your more fleeting ones at the other, can mean reduced allocation times. It may not. Your mileage may vary. The search options are provided so that you may take advantage of any inherent knowledge of your own program's dataflow.

In terms of memory *fragmentation*, general speaking you will find there to be little practical difference between allocating everything at one end of the address space, versus allocating both both ends - fragmentation under both of these strategies is more likely to be a result of your program's allocation/deallocation pattern. Of course you will be able to fabricate a situation where that isn't true, and that's fine. Most of the time, you can be fairly safe with this assumption until your own careful analysis of your program determines otherwise.

`MiddleUp`/`MiddleDown`

There are two additional search strategies, `MiddleUp` and `MiddleDown` that work as they imply. The downside to their use is the *guaranteed fragmentation of memory*. The instant you allocate even a single page in the middle of the address space, you've more or less halved the size of the largest contiguous block of memory you can allocate, until you deallocate it.

These two extra strategies exist for those situations where you won't be allocating anything massive, but do want the additional little areas where you know the searches will be shorter, because of how you've planned your memory usage.

**NOTE:** Because of the way `MiddleUp` and `MiddleDown` work, algorithmically, they can only search half of the address space (lower half or upper half). They can't "wrap around" because the allocated space must be contiguous. Due to this, when you call `memory::allocate()` with either of these `Middle` strategies and the search for free memory fails, the allocator will automatically renew the search in the other half of memory using the other `Middle` strategy. This is done to ensure that no matter which strategy you use, if the required sized chunk is available somewhere, it will be found.
