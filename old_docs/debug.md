# Debug
```zero/core/debug.h```

The ```debug``` class provides a simple (non-ISR) software TX for debugging output.

It is intended to be a very lightweight method of outputting data to a terminal for **debugging purposes**, even on devices lacking a hardware UART.

## debug::print() - characters and null-terminated strings
Outputs a character or string to the ```DEBUG_PORT``` on ```DEBUG_PIN``` at ```DEBUG_BAUD``` (see ```makefile```).
```
    static void debug::print(char c)

    static void debug::print(
        char* s,
        const bool fromFlash = false
        )
```

### Parameters
|Param|Description|
|-----|-----------|
|```c```|The character to transmit, or...|
|```s```|The NULL-terminated string to transmit.|
|```fromFlash```|Specifies that ```s``` points to an address in Flash memory (see ```PROGMEM``` macro in AVR-libc).|

## debug::print() - integers
Outputs a 16-bit integer number in a specified base.
```
    static void debug::print(
        const uint16_t n,
        const int base = 10
    )
```

### Parameters
|Param|Description|
|-----|-----------|
|```n```|The number to output|
|```base```|Optional base for the number. **Default: 10**.|

### Notes
These are static methods. These are also blocking calls - control returns to the caller only after the transmission is complete.

Interrupts are disabled while each character is transmitted in a tight-loop (approximately 1ms per character at 9600bps). Interrupts are enabled between each character.

No set up or initialization is required by your code - just be sure that the ```makefile``` has the ```DEBUG_*``` settings to your liking, and then call these functions.

For asynchronous and better performing data transmission, see the ```UsartTx``` or ```SuartTx``` classes.

## debug::assert()
Outputs debugging information if a condition is ```false```.
```
    static void debug::assert(
        const bool c,
        const char* const msg,
        const int line
    )
```

### Parameters
|Param|Description|
|-----|-----------|
|```c```|The condition to test.|
|```msg```|The null-terminated string to output if ```c``` is false.|
|```line```|The line number in the source code of the assertion.|

### Notes
If ```c``` is ```false```, then ```msg``` is sent to ```debug::print```, along with some information about the ```Thread``` and the line number of the issue.
