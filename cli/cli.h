//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_CLI_H
#define TCRI_ZERO_CLI_H


#include <stdint.h>
#include "usart.h"


typedef zero::UsartTx CliTx;
typedef zero::UsartRx CliRx;

int cliEntry();

const auto CLI_CMD_LINE_MAX_TOKENS = 16;


#endif