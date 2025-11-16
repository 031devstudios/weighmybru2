#pragma once

#ifdef __has_include
  #if __has_include("my_config.h")
    #include "my_config.h"
  #endif
#endif

#ifndef DISPLAY_WIDTH
  #define DISPLAY_WIDTH 128
#endif

#define DISPLAY_HEIGHT_32 32
#define DISPLAY_HEIGHT_64 64

#define DISPLAY_CONTROLLER_SSD1306 1
#define DISPLAY_CONTROLLER_SH1106  2

#ifndef DISPLAY_CONTROLLER
#define DISPLAY_CONTROLLER DISPLAY_CONTROLLER_SSD1306
#endif

#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT DISPLAY_HEIGHT_32  
#endif