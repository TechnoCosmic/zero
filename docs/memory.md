# Memory
```zero/memory.h```

The ```memory``` namespace handles the management of dynamically allocated SRAM.

## memory::allocate()
Allocates SRAM for use by the caller.

```
    uint8_t* memory::allocate(
        const uint16_t bytesReqd,
        uint16_t* allocatedBytes,
        SearchStrategy strategy
        )
```
### Parameters
|Param|Description|
|-----|-----------|
|```bytesReqd```|The number of bytes required.|
|```allocatedBytes```|A place to store the actual number of bytes assigned by the kernel. May be null.|
|```strategy```|How should the heap be searched? ```BottomUp``` or ```TopDown```?|

### Notes
- Because zero uses a page-based memory allocator, the number of bytes actually allocated by any call to ```allocate()``` will always be a multiple of the page size (default is 16 bytes, adjustable in the ```makefile```). If your program can make use of any 'extra' bytes assigned, you may pass a pointer to a ```uin16_t``` as the second argument to learn how many bytes you can actually use.

### Search Strategy

|Flag|Description|
|----|-----------|
|```BottomUp```|The allocator searchs for free memory, starting at the bottom of it's heap, and working up towards higher addresses.|
|```TopDown```|The allocator searchs for free memory, starting at the top of it's heap, and working down towards lower addresses.|

```SearchStrategy``` exists to support a very simple page-based allocation concept, rather than the traditional free-list implementation. For this reason, allocations are more costly in terms of time, but the benefit is less complicated code. By implementing the idea of a search strategy, you can compensate for the more costly allocation by having short-lived blocks of memory always allocated at one end, and longer-lived stuff at the other. This will reduce allocation time if used appropriately.

### Example
```
#include "memory.h"

int memoryDemo()
{
    uint16_t allocatedBytes = 0UL;
    char* myBuffer = (char*) memory::allocate(35, &allocatedBytes, memory::SearchStrategy::TopDown);

    // do stuff with the memory here...

    // free it
    memory::free(myBuffer, allocatedBytes);
    myBuffer = 0UL;
    allocatedBytes = 0UL;
}
```

## memory::free()
Returns a previously allocated chunk of memory to the heap.
```
    void memory::free(
        const uint8_t* address,
        uint16_t numBytes
        )
```
See ```allocate()``` for details.

## ```new``` and ```delete```
zero provides overrides for both of these operators, but does not support allocating/deallocating arrays of things. Use ```allocate()``` and ```free()``` described above for arrays instead.
