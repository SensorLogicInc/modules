/**
@file project.h

Definitions for various project related constants

@note
virtual_com.c contains main()

@par Environment
Environment Independent

@par Compiler
Compiler Independent

@author Justin Hadella

@copyright (c) 2021 Sensor Logic
*/
#ifndef PROJECT_h
#define PROJECT_h

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define FW_NAME "vcom_xep_matlab_server"
#define FW_VER  "1.0.0"

#define RGB_COLOR_OFF    (0x0)
#define RGB_COLOR_RED    (0xff0000)
#define RGB_COLOR_GREEN  (0x00ff00)
#define RGB_COLOR_BLUE   (0x0000ff)
#define RGB_COLOR_YELLOW (0xffff00)
#define RGB_COLOR_CYAN   (0x00ffff)
#define RGB_COLOR_VIOLET (0x220033)

#ifdef __cplusplus
}
#endif
#endif // PROJECT_h
