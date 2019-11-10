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
#include <avr/io.h>

namespace zero {

    // gather statistics regarding threads and memory and stuff
    #define INSTRUMENTATION

    // CLI-enabled?
    #define CLI_ENABLED

// CLI-specific stuff in here...
#ifdef CLI_ENABLED

    // CLI USART speed
    const uint32_t CLI_BAUD = 57600;

    // CLI Terminal Type
    #define CLI_VT100

    // CLI RX Pipe buffer bytes
    const uint16_t CLI_RX_PIPE_BYTES = 32;

    // CLI TX Pipe buffer bytes
    const uint16_t CLI_TX_PIPE_BYTES = 96;

    // CLI stack size
    const uint16_t CLI_STACK_BYTES = 384;

    // CLI command line buffer size
    const uint16_t CLI_CMD_LINE_BUFFER_BYTES = 40;

    // Maximum number of tokens on a CLI command line
    const uint16_t CLI_CMD_LINE_MAX_TOKENS = 16;

    // Built-in CLI commands can be easily enabled/disabled
    // individually here so that you don't need to mess
    // around with source files and such.
    
    // Process Status and Uptime commands - ps/uptime
    #define CLICMD_PS

    // Memory map command - memmap
    #define CLICMD_MEMMAP

    // Memory dump commands - ram/flash/eeprom
    #define CLICMD_MEMDUMP

    // List command - ls
    #define CLICMD_LS

    // Thread control commands - pause/play
    #define CLICMD_THREAD_CTRL

#endif

    // Default quantum, in milliseconds
	const int TIMESLICE_MS = 15;

    // Allocator search strategy for Threads and Stacks
    #define THREAD_MEMORY_SEARCH_STRATEGY memory::SearchStrategy::TopDown

    // So that the allocator scales with SRAM
    // NOTE: Change this to a simple fixed size if you prefer
    const uint16_t DYNAMIC_BYTES = RAMEND - (255 + 1024);

    // Idle thread stack size
    // NOTE: This may be bumped up if it is below the minimum
    // stack size required (found at the top of thread.cpp)
    const uint16_t IDLE_THREAD_STACK_BYTES = 96;

    // Memory allocator page size
    const uint8_t PAGE_BYTES = 16;

    // leave this one alone
    const uint16_t SRAM_PAGES = DYNAMIC_BYTES / PAGE_BYTES;

    // Minimum size of a dynamic Pipe buffer, in bytes
    const uint16_t MINIMUM_PIPE_BYTES = PAGE_BYTES;

}

#endif
