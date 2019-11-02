/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_CONFIG_H
#define TCRI_ZERO_CONFIG_H

#include <stdint.h>

namespace zero {

    // gather statistics regarding threads and memory and stuff
    #define INSTRUMENTATION

    // CLI-enabled?
    #define CLI_ENABLED

// CLI-specific stuff in here...
#ifdef CLI_ENABLED

    // CLI USART speed
    const uint32_t CLI_BAUD = 57600;

    // CLI RX Pipe buffer bytes
    const uint16_t CLI_RX_PIPE_BYTES = 32;

    // CLI TX Pipe buffer bytes
    const uint16_t CLI_TX_PIPE_BYTES = 96;

    // CLI stack size
    const uint16_t CLI_STACK_BYTES = 512;

    // CLI command line buffer size
    const uint16_t CLI_CMD_LINE_BUFFER_BYTES = 40;

    // Maximum number of tokens on a CLI command line
    const uint16_t CLI_CMD_LINE_MAX_TOKENS = 16;

#endif

    // Kernel (context switcher) stack size
    const uint16_t KERNEL_STACK_BYTES = 128;

    // How much lower memory is your program using for globals?
    const uint16_t GLOBALS_BYTES = 384;

    // Idle thread stack size
    // NOTE: This may be bumped up if it is below the minimum
    // stack size required (found at the top of thread.cpp)
    const uint16_t IDLE_THREAD_STACK_BYTES = 32;

    // Flashes an LED on any pin on PORTC when the idle thread is running
    // #define IDLE_BLINK

    // Memory allocator page size
    const uint8_t PAGE_BYTES = 32;

}

#endif
