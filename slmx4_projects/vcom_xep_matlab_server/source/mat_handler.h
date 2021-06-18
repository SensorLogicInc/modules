/**
@file mat_handler.h

Definitions for the Matlab connector data handler

@par Environment
Environment Independent

@par Compiler
Compiler Independent

@author Justin Hadella

@copyright (c) 2021 Sensor Logic
*/
#ifndef MAT_HANDLER_h
#define MAT_HANDLER_h

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define MAT_HANDLER_VERSION "1.0.0"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Public Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
Function to initialize components needed in order to handle client requests
when connected
*/
void handle_client_init();

/**
Function to handle client requests

In this version, the client task is a basic TCP server. The server acts like an
infinite loop where it will check to see if there are any commands from the user
to handle. It also will do things like stream the radar data.
*/
void handle_client_request(uint8_t *buf, int n);

#ifdef __cplusplus
}
#endif
#endif // MAT_HANDLER_h
