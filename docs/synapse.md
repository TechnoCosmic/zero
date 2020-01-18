# Synapse
```zero/core/thread.h```

```Synapse``` is simple class that pairs a ```Thread*``` with a ```SignalField```, in order to make it easy to pass around an object representing the endpoint, or target, of a signal.

## Constructors
```
    Synapse()

    Synapse(
        const SignalField sigs
        )
```
A ```Synapse``` has no private members. It is a very transparent helper class. You can directly access ```Synapse::thread``` and ```Synapse::signals```.

The constructor with the ```SignalField``` parameter is what lets us pass a ```SignalField``` where a ```Synapse``` is expected. The compiler will construct a ```Synapse``` automatically, and will use the currently executing ```Thread``` as the thread component of the ```Synapse```. See ```Receiver::enable()``` as one example of this.

Generally I'd frown on this sort of implicit conversion, but it enhances readability and maintainablity of the code significantly.

The intention is that instead of code directly calling ```Thread::signal()```, it should call ```Synapse::signal()```. By adopting this convention, it becomes very easy to create and pass these endpoints around without your program having to manage it.

### Notes
Synapses should always be handed around by copy, rather than by reference or by pointer. On the target MCUs, a ```Synapse``` is a 32-bit number, effectively, and since they're not (or shouldn't be) passed around (and hence copied) often in time-critical code, the benefit of simplied use is worth it.

## signal()
Signals the ```Thread``` specified in the ```Synapse``` with the signals in the ```Synapse's``` ```signals``` field.
```
    void Synapse::signal()
```

### Notes
Calling ```Synapse::signal()``` is functionally equivalent to the following code...
```
    mySynapse.thread->signal(mySynapse.signals);
```
... except with validation so we don't signal a null ```Thread*``` and so on.

## clearSignals()
Calls ```Thread::clearSignals()``` with the signals associated with the ```Synapse```.
```
    void Synapse::clearSignals()
```

## clear()
Clears both the ```thread``` and ```signals``` fields.
```
    void clear()
```

## isValid()
Returns ```true``` if the ```Synapse``` has valid ```thread``` *and* ```signals``` fields.
```
    bool Synapse::isValid()
```
