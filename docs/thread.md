# startup_sequence()
```
    int startup_sequence()
```

Because zero has hijacked ```main()``` for it's own initialization purposes, your program will use ```startup_sequence()``` to initialize itself and spawn your initial threads. Code inside ```startup_sequence()``` does NOT run in the context of a Thread, so don't block, don't do anything complicated. Set up you initial state and spawn your initial ```Threads```.

# Thread
```zero/core/thread.h```

The ```Thread``` class handles the management of individual Threads within the zero kernel.

## Constructor
Constructs a new Thread object, and begins executing it.
```
    Thread::Thread(
        const char* const name,
        const uint16_t stackBytes,
        const ThreadEntry entryPoint,
        const ThreadFlags flags,
        const Synapse* const termSyn,
        int* exitCode
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```name```|Name of the Thread. May be null. If non-null, points to a string in Flash memory.|
|```stackBytes```|Size of the Thread's stack, in bytes. A minimum of 128 bytes will be allocated for the stack.|
|```entryPoint```|The function that contains the main body of the Thread's code.|
|```flags```|A bitfield controlling aspects of the thread's behavior. **DEFAULT:** ```TF_READY```|
|```termSyn```|A ```Synapse``` to signal when this ```Thread``` terminates. Optional.|
|```exitCode```|A pointer to an ```int``` to store the return code from the Thread's ```entry``` function. Optional.|

### Notes
- The ```entryPoint``` is a function that takes no parameters and returns an ```int```. Conforms to ```ThreadEntry```.

- ```flags```...

|Flag|Description|
|----|-----------|
|```TF_NONE```|No flags specified.|
|```TF_READY```|Makes the Thread immediately available to execute.|

- If you want to know when a child Thread terminates, allocate a signal via a ```Synapse``` and pass that as ```termSyn```. When the child Thread terminates, that ```Synapse``` will be signalled, and can be checked by calling ```getCurrentSignals()``` or by ```wait()```ing on it.

- ```exitCode``` is only valid once (and if) a Thread terminates.

### Example
```
#include <avr/pgmspace.h>
#include "thread.h"
#include "synapse.h"

int myAsyncThread()
{
    // do other work here...

    // return the result
    return 42;
}

int startup_sequence()
{
    // do work here...

    // now we're going to launch another Thread
    // from here to do some asynchronous work

    // this is where the child Thread will store it's return code
    int asyncReturnCode{ 0 };

    // this Synapse will be used to learn when the child Thread terminates
    Synapse asyncTermSyn;

    // define and launch the Thread
    new Thread{
        PSTR( "asyncDemo" ),
        192,
        myAsyncThread,
        TF_READY,
        &asyncTermSyn,
        &asyncReturnCode };

    // do other work while that's going
    // ...
    // ...

    // now we want the results from the async Thread
    
    // wait for the Thread to finish.
    // this will block if the Thread is still running
    termSyn.wait();

    // use the results...
    // ...

    // ... and exit
    return 0;
}
```

## fromPool()
This static method returns a ```Thread``` from the system thread pool, if one is available. The total number of pool threads is set in the ```makefile``` - search for ```NUM_POOL_THREADS```. Right next to that setting is ```POOL_THREAD_STACK_BYTES```, which sets the size of the stack for pool threads.
```
    static Thread* Thread::fromPool(
        const char* const name,
        const ThreadEntry entryPoint,
        const Synapse* const termSyn,
        int* exitCode
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```name```|Name of the Thread. May be null. If non-null, points to a string in Flash memory.|
|```entryPoint```|The function that contains the main body of the Thread's code.|
|```termSyn```|A ```Synapse``` to signal when this ```Thread``` terminates. Optional.|
|```exitCode```|A pointer to an ```int``` to store the return code from the Thread's ```entry``` function. Optional.|

### Notes
If no ```Threads``` are currently available in the thread pool, this method will return ```nullptr```. Otherwise, a pointer to the ```Thread``` will be returned and it will immediately begin executing.

Pool threads are not intended to tasks that never exit - use a dedicated thread for those cases. Pool threads are best used for asychronous functions (as pool threads are faster to spin up) and in situations where you don't necessarily have the SRAM available for more dedicated threads.

## getCurrent()
Returns the currently executing ```Thread``` object.
```
    static Thread& Thread::getCurrent()
```

### Notes
This is a static method of the ```Thread``` class. For readability of code, use the ```me``` macro instead.

## forbid()
Prevents context switching.
```
    static void Thread::forbid()
```

### Notes
This is a static method of the ```Thread``` class. Use this sparingly, for critical sections where context switching needs to be prevented temporarily.

**NOTE:** This only suspends context switching (multi-tasking) - it does NOT disable interrupts.
**NOTE:** ```forbid()``` and ```permit()``` do NOT nest - calling ```forbid()``` once will prevent context switching no matter how many times ```permit()``` was called prior. For a nested mechanism, see the ```ZERO_ATOMIC_BLOCK``` macro.

## permit()
Resumes context switching.
```
    static void Thread::resume()
```

### Notes
This is a static method of the ```Thread``` class. Use this to resume context switching after having previously called ```forbid()```.

**NOTE:** ```forbid()``` and ```permit()``` do NOT nest - calling ```permit()``` once will resume context switching no matter how many times ```forbid()``` was called prior. For a nested mechanism, see the ```ZERO_ATOMIC_BLOCK``` macro.

## isSwitchingEnabled()
Determines if context switching is currently enabled.
```
    static bool Thread::isSwitchingEnabled()
```

### Notes
Returns ```true``` if context switching is currently enabled, ```false``` otherwise.

## now()
Returns a 32-bit number representing the number of milliseconds elapsed since the zero kernel was initialized.
```
    static uint32_t Thread::now()
```
### Notes
Wraps around (overflows) after approximately 49 days of continuous operation.

## getThreadId()
Returns the unique ID of the Thread.
```
    uint16_t Thread::getThreadId() const
```

## getName()
Returns a pointer into Flash memory that contains the name of the Thread.
```
    const char* Thread::getName() const
```

## allocateSignal()
*** NOTE: *** You cannot use ```allocateSignal()``` and ```freeSignals()``` directly - use ```Synapse``` to manage signals instead. These methods are private to the ```Thread``` and are presented for documentation purposes only.

Allocates a signal for use.
```
    SignalField Thread::allocateSignal(
        const uint16_t reqdSignalNumber = -1
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```reqdSignalNumber```|If you require a specific signal number for any reason, specify it here (1-15). Omit this parameter to let zero allocate the first available signal number. Optional.|

### Notes
This function returns a ```SignalField``` that represents the allocated signal. Returns ```0``` if all signals are currently allocated.

- A ```SignalField``` is just a ```uint16_t``` used as a bitfield for signals.
- Free the signal when no longer needed by calling ```freeSignals()```.
- ```SIG_TIMEOUT``` is a reserved signal. The remaining 15 are available for program use.

***NOTE:*** Do not call ```allocateSignal()``` or ```freeSignals()``` directly. Instead, create a ```Synapse``` on the stack and use it wherever a ```Synapse``` or ```SignalField``` would be expected. See the example code throughout the documentation for usage.

## freeSignals()
*** NOTE: *** You cannot use ```allocateSignal()``` and ```freeSignals()``` directly - use ```Synapse``` to manage signals instead. These methods are private to the ```Thread``` and are presented for documentation purposes only.

Frees a previously allocated signal(s) for re-use.
```
    void Thread::freeSignals(
        const SignalField signals
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```signals```|The ```SignalField``` of the signal(s) you want to deallocate. Can free multiple signals at once.|

***NOTE:*** Do not call ```allocateSignal()``` or ```freeSignals()``` directly. Instead, create a ```Synapse``` on the stack and use it wherever a ```Synapse``` or ```SignalField``` would be expected. See the example code throughout the documentation for usage.

## getCurrentSignals()
Returns the signals currently set for the Thread.
```
    SignalField Thread::getCurrentSignals()
```

### Notes
While it is expected that a Thread will usually ```wait()``` on one or more signals, it occasionally may be necessary to opportunistically check the state of one or more signals. This is the function that will do that, returning a ```SignalField``` that contains the signals that are currently set.

## clearSignals()
Clears one or more signals, returning the remaining set signals.
```
    SignalField Thread::clearSignals(
        const SignalField sigs
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```sigs```|The ```SignalField``` containing the signal(s) to be cleared.|

### Notes
If a signal occurs while your Thread is awake, and you check it with ```getCurrentSignals()```, you will usually want to clear those signals after you have acted on them, to prevent your code from erroneously acting on them again. Use this function to tell zero that you've addressed some signals outside of the normal ```wait()``` process. If you do not do this, and you ```wait()``` on these signals again, your Thread will NOT block as those signals are still set.

You do NOT need to call ```clearSignals()``` after your Thread wakes from ```wait()``` as the signals that woke the Thread are automatically cleared by the kernel.

## wait()
Waits for one or more signals to occur, blocking if necessary.
```
    SignalField Thread::wait(
        const SignalField sigs,
        const uint32_t timeoutMs = 0UL
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```sigs```|The signal(s) to wait for. You may specify multiple signals in the one ```SignalField``` by using bitwise-OR.|
|```timeoutMs```|An optional timeout for the call, in milliseconds.|

### Notes
Returns a ```SignalField``` specifying which signals woke the Thread up. May be more than one if more than one signal was ```wait()```ed on and multiple occurred, so be sure to check for all signals when the Thread wakes.

You do NOT need to call ```clearSignals()``` on any signal that you ```wait()```ed on, as they will be cleared automatically when your Thread wakes up.

Also be aware that a call to ```wait()``` does not necessarily block - if any of the signals you are waiting on are already set prior to calling ```wait()```, your code will continue executing without blocking. This means you can safely call ```wait()``` when needed and you will get the best performance in both pre-set and blocking situations.

#### ```timeoutMs```
This can be used to optionally provide a timeout for the call. If you only want a thread-friendly blocking delay (without waiting on any other signals), just use...
```
    auto wokeSigs{ me.wait( SIG_TIMEOUT, 500 ) };

    if ( wokeSigs & SIG_TIMEOUT ) {
        // ...
    }
```
... replacing ```500``` with your desired delay. ```SIG_TIMEOUT``` is the reserved ```SignalField``` for timeouts. Alternatively, you can use ```::delay()``` (see below).

If you are waiting on other signals and want a timeout as well, you do not need to specify the ```SIG_TIMEOUT``` flag, but you can if you want to, for clarity.

## delay()
Blocks for a given number of milliseconds.
```
    void Thread::delay(
        const uint32_t ms
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```ms```|The amount of time to block, in milliseconds.|

### Notes
Equivalent to calling ```::wait( 0, ms )```.

## signal()
Signals a Thread, potentially waking it up.
```
    void Thread::signal(
        const SignalField sigs
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```sigs```|The signal(s) that you want to send to the Thread.|

### Notes
Threads are woken from ```wait()``` with use of the ```signal()``` function. ```signal()``` is usually called by another Thread, or device driver.

## getPeakStackUsage()
Returns the peak recorded stack usage for the ```Thread```.
```
    uint16_t getPeakStackUsage() const;
```

### Notes
The stack usage is only checked when a ```Thread``` is pre-empted or voluntarily yields, for the sake of efficiency. This means that actual peak stack usage may be higher than reported here.

# Macros
## ZERO_ATOMIC_BLOCK
Similar to AVR-libc's ```ATOMIC_BLOCK``` macros, this macro lets you easily wrap up a critical section that calls ```Thread::forbid()``` and ```Thread::permit()``` for you...

```
// do normal stuff here...
// ...
// ...

ZERO_ATOMIC_BLOCK( ZERO_ATOMIC_RESTORESTATE ) {

    // do stuff here that requires context-switching
    // to be disabled in order to work correctly

    // ...
    // ...    

}
```
There is also a zero equivalent of ```ATOMIC_FORCEON```, called ```ZERO_ATOMIC_FORCEON```, that behaves as you would expect.
