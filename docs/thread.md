# Thread
```zero/thread.h```

The ```Thread``` class handles the management of individual Threads within the zero kernel.

## Constructor
Constructs a new Thread object, and begins executing it.

```
    Thread::Thread(
        const uint16_t stackBytes,
        const ThreadEntry entryPoint,
        const ThreadFlags flags,
        const SignalField termSignals,
        uint16_t* exitCode
        )
```
### Parameters
|Param|Description|
|-----|-----------|
|```stackBytes```|Size of the Thread's stack, in bytes. A minimum of 96 bytes will be allocated for the stack.|
|```entryPoint```|The function that contains the main body of the Thread's code.|
|```flags```|A bitfield controlling aspects of the Thread's behavior. **DEFAULT:** ```TF_READY```|
|```termSignals```|Signals to set in the parent Thread when this new Thread terminates. Optional.|
|```exitCode```|A pointer to a ```uint16_t``` to store the return code from the Thread's ```entry``` function. Optional.|

### Notes
- The ```entryPoint``` is a function that takes no parameters and returns an ```int```. Conforms to ```ThreadEntry```.

- ```flags```...

|Flag|Description|
|----|-----------|
|```TF_READY```|Makes the Thread immediately available to execute|
|```TF_SELF_DESTRUCT```|Will clean up the Thread object after termination, and is for use in cases where you cannot call ```delete``` on the Thread object when its finished, such as those Threads launched from ```startup_sequence()``` (the ```main()``` replacement function for zero programs)
|```TF_FIRE_AND_FORGET```|Shortcut for both ```TF_READY``` and ```TF_SELF_DESTRUCT``` combined|

- If you want to know when a child Thread terminates, allocate a signal via ```allocateSignal()``` and pass that as ```termSignals```. When the child Thread terminates, that signal will be set, and can be checked by calling ```getCurrentSignals()``` or by ```wait()```ing on it.

- ```exitCode``` is only valid once (and if) a Thread terminates.

### Example
```
#include "thread.h"

int myAsyncThread()
{
    // do other work here...

    // return the result
    return 42;
}

int myFirstThread()
{
    // do work here...

    // now we're going to launch another Thread
    // from here to do some asynchronous work

    // this is where the child Thread will store it's return code
    int asyncReturnCode = 0;

    // this signal will be used to learn when the child Thread terminates
    SignalField asyncTermSig = me.allocateSignal();

    // define and launch the Thread
    Thread* async = new Thread(
        192,
        myAsyncThread,
        TF_READY,
        asyncTermSig,
        &asyncReturnCode);

    // do other work while that's going
    // ...
    // ...

    // now we want the results from the async Thread
    
    // wait for the Thread to finish.
    // this will block if the Thread is still running

    if (me.wait(asyncTermSig) & asyncTermSig) {
        // asyncReturnCode now holds the
        // results from the async Thread

        // use the results...
        // ...

        // and because we didn't use
        // TF_SELF_DESTRUCT, we must delete
        // the Thread ourselves...
        
        delete async;
        async = 0UL;

        // and we don't need the termination signal
        // anymore, so we can free that up also

        me.freeSignals(asyncTermSig);
        asyncTermSig = 0UL;
    }

    // ...and exit
    return 0;
}

void startup_sequence()
{
    // start one simple Thread when the MCU fires up
    new Thread(128, myFirstThread, TF_FIRE_AND_FORGET);
}
```

## getCurrentThread()
Returns the currently executing ```Thread``` object.
```
    static Thread* Thread::getCurrentThread()
```

### Notes
This is a static method of the ```Thread``` class. For readability of code, use the ```me``` macro instead, which returns a *reference* to the current ```Thread``` object, rather than a pointer.

## forbid()
Prevents context switching.
```
    static void Thread::forbid()
```

### Notes
This is a static method of the ```Thread``` class. Use this sparingly, for critical sections where context switching needs to be prevented temporarily.

## permit()
Resumes context switching.
```
    static void Thread::resume()
```

### Notes
This is a static method of the ```Thread``` class. Use this to resume context switching after having previously called ```forbid()```.

## isSwitchingEnabled()
Determines if context switching is currently enabled.
```
    static bool Thread::isSwitchingEnabled()
```

### Notes
Returns ```true``` if context switching is currently enabled, ```false``` otherwise.

## now()
Returns a 64-bit number representing the number of milliseconds elapsed since the zero kernel was initialized.
```
    static uint64_t Thread::now();
```

## allocateSignal()
Allocates a signal for use.
```
    SignalField Thread::allocateSignal(
        const uint16_t reqdSignalNumber = -1
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```reqdSignalNumber```|If you require a specific signal number for any reason, specify it here (0-15). Omit this parameter to let zero allocate the first available signal number. Optional.|

### Notes
This function returns a ```SignalField``` that represents the allocated signal. Returns ```0``` if all signals are currently allocated.

A ```SignalField``` is just a ```uint16_t``` used as a bitfield for signals.

Free the signal when no longer needed by calling ```freeSignals()```.

## freeSignals()
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
        const SignalField sigs
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```sigs```|The signal(s) to wait for. You may specify multiple signals in the one ```SignalField```.|

### Notes
Returns a ```SignalField``` specifying which signals woke the Thread up. May be more than one if more than one signal was ```wait()```ed on and multiple occurred, so be sure to check for all signals when the Thread wakes.

You do NOT need to call ```clearSignals()``` on any signal that you ```wait()```ed on, as they will be cleared automatically when your Thread wakes up.

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
