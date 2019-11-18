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


extern int cliMain();


namespace zero {

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