# Debug
```zero/debug.h```

The ```debug``` class provides a simple (non-ISR) software TX for debugging output.

It is intended to be a very lightweight method of outputting data to a terminal for **debugging purposes**, even on devices lacking a hardware UART.

## debug::print()
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

### Notes
These are static methods. These are also blocking calls - control returns to the caller only after the transmission is complete.

Interrupts are disabled while each character is transmitted in a tight-loop (approximately 1ms per character at 9600bps). Interrupts are re-enabled between each character.

No set up or initialization is required by your code - just be sure that the ```makefile``` has the ```DEBUG_*``` settings to your liking, and then call these functions.

For more comprehensive and better performing data transmission, see the ```UsartTx``` or ```SuartTx``` classes.
