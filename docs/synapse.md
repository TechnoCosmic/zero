# Synapse
```zero/core/thread.h```

```Synapse``` is a guard class for allocating and freeing signals. It pairs a ```Thread*``` with a ```SignalField```, and provides RAII/SBRM allocation of signals.

## Constructor
```
    Synapse()
```
Constructing a ```Synapse``` will allocate a signal. When the ```Synapse``` goes out-of-scope, the signal will be freed.

### Notes
Synapses should always be handed around by reference or by pointer, as the copy constructor has been deleted.

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

## wait()
Waits for the signal(s) represented by the ```Synapse```, blocking if required. 
```
    SignalField Synapse::wait(
        const uint32_t timeoutMs = 0ULL
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```timeoutMs```|An optional timeout for the call, in milliseconds.|

### Notes
This is shorthand for ```me.wait(Synapse::signals, timeoutMs)```, and only works if the caller is the ```Thread``` represented by the ```Synapse```. If you need to wait on multiple signals/```Synapses``` simultaneously, call ```Thread::wait()``` specifying all the required signals.

## Example
```
int myThread
{
    Synapse txReadySyn;
    UsartTx tx( 0 );

    tx.setCommsParams( 9600 );
    tx.enable( txReadySyn );

    while (true) {
        txReadySyn.wait();

        // transmit something...
        tx.transmit( "Hello!\r\n", 8, false );
    }

    // don't need to do anything with the UsartTx or
    // Synapse objects, since they'll both fall out-of-
    // scope here and automatically free their resources
}
```
