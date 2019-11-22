/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_CLI_H
#define TCRI_ZERO_CLI_H

#include <avr/pgmspace.h>
#include "namedobject.h"
#include "textpipe.h"


// this is the Thread entry point for the CLI itself, should you wish to launch the CLI
extern int cliMain();

#define CLI_RX "cli_rx"
#define CLI_TX "cli_tx"

namespace zero {

    // This is the function signature for a CLI command
    typedef int (*CliEntryPoint)(TextPipe* rx, TextPipe* tx, int argc, char* argv[]);

    class CliCommand {
    public:
        CliCommand(const char* name, const CliEntryPoint entryPoint);
        int execute(TextPipe* rx, TextPipe* tx, int argc, char* argv[]);
        static void shell(const char* commandLine);

    private:
		// NamedObject must be first
        NamedObject _systemData;
        CliEntryPoint _entryPoint;
    };

#ifdef CLI_ENABLED
	// helper macro for easier Thread creation
	#define clicommand(v,fn)										\
		const PROGMEM char _cmdName_##v[] = #v;	                	\
		CliCommand v(_cmdName_##v,[]fn)
#else
    #define clicommand(v,fn) ;
#endif

}

#endif