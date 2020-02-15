# Memory
```zero/core/memory.h```

The ```memory``` namespace handles the management of dynamically allocated SRAM.

The amount of SRAM available to zero's allocator is defined in the ```makefile```. Search for ```DYNAMIC_BYTES```, making sure you're adjusting the correct one for your chosen target MCU.

## memory::allocate()
Allocates SRAM for use by the caller.

```
    void* memory::allocate(
        const uint16_t bytesReqd,
        uint16_t* allocatedBytes = nullptr,
        SearchStrategy strategy = SearchStrategy::BottomUp
        )
```
### Parameters
|Param|Description|
|-----|-----------|
|```bytesReqd```|The number of bytes required.|
|```allocatedBytes```|A place to store the actual number of bytes assigned by the kernel. Optional, may be null.|
|```strategy```|How should the heap be searched? ```BottomUp``` or ```TopDown```? Optional.|

### Notes
- Because zero uses a page-based memory allocator, the number of bytes actually allocated by any call to ```allocate()``` will always be a multiple of the page size (default is 16 bytes, adjustable in the ```makefile```, search for ```PAGE_BYTES```). If your program can make use of any 'extra' bytes assigned, you may pass a pointer to a ```uint16_t``` as the second argument to learn how many bytes you can actually use.

### Search Strategy

|Value|Description|
|-----|-----------|
|```BottomUp```|The allocator searches for free memory, starting at the bottom of it's heap, and working up towards higher addresses.|
|```TopDown```|The allocator searches for free memory, starting at the top of it's heap, and working down towards lower addresses.|

```SearchStrategy``` exists to support a very simple page-based allocation concept, rather than the traditional free-list implementation. For this reason, allocations are more costly in terms of time, but the benefit is less complicated code. By implementing the idea of a search strategy, you can compensate for the more costly allocation by having short-lived blocks of memory always allocated at one end of the address space, and longer-lived stuff at the other. This will reduce allocation time if used appropriately.

### Example
```
#include "memory.h"

int memoryDemo()
{
    uint16_t allocatedBytes = 0;
    auto myBuffer{ (char*) memory::allocate( 35, &allocatedBytes, memory::SearchStrategy::TopDown ) };

    // do stuff with the memory here...

    // free it
    memory::free( myBuffer, allocatedBytes );
    myBuffer = nullptr;
    allocatedBytes = 0;
}
```

## memory::free()
Returns a previously allocated chunk of memory to the heap.
```
    void memory::free(
        const void* address,
        uint16_t numBytes
        )
```
See ```allocate()``` for details.

## ```new``` and ```delete```
zero provides overrides for both of these operators, but does not support allocating/deallocating arrays of things. Use ```allocate()``` and ```free()``` described above for arrays instead.
