#ifndef LOGLEVELS_H
#define LOGLEVELS_H

#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h" // needed for call to xTaskGetTickCount()
#include "task.h"     // needed for call to xTaskGetTickCount()

enum {
  LOG_NONE,
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG,
  LOG_VERBOSE,
};

// Define a default log level.
// Each source file can override this by defining it 
// prior to including this file.
#ifndef LOG_LEVEL_MODULE
#define LOG_LEVEL_MODULE  LOG_VERBOSE
#endif

// ANSI logging colors, inspired by ESP32's OSS log library. 
#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D       LOG_COLOR(LOG_COLOR_CYAN)
#define LOG_COLOR_V       LOG_COLOR(LOG_COLOR_PURPLE)

#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%lu) %s: " format LOG_RESET_COLOR "\n"
#define LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==LOG_ERROR )          { printf(LOG_FORMAT(E, format), xTaskGetTickCount(), tag, ##__VA_ARGS__); } \
        else if (level==LOG_WARN )      { printf(LOG_FORMAT(W, format), xTaskGetTickCount(), tag, ##__VA_ARGS__); } \
        else if (level==LOG_DEBUG )     { printf(LOG_FORMAT(D, format), xTaskGetTickCount(), tag, ##__VA_ARGS__); } \
        else if (level==LOG_VERBOSE )   { printf(LOG_FORMAT(V, format), xTaskGetTickCount(), tag, ##__VA_ARGS__); } \
        else                            { printf(LOG_FORMAT(I, format), xTaskGetTickCount(), tag, ##__VA_ARGS__); } \
    } while(0)

#define LOGE( tag, format, ... ) LOG_LEVEL_RUNTIME(LOG_ERROR,   tag, format, ##__VA_ARGS__)
#define LOGW( tag, format, ... ) LOG_LEVEL_RUNTIME(LOG_WARN,    tag, format, ##__VA_ARGS__)
#define LOGI( tag, format, ... ) LOG_LEVEL_RUNTIME(LOG_INFO,    tag, format, ##__VA_ARGS__)
#define LOGD( tag, format, ... ) LOG_LEVEL_RUNTIME(LOG_DEBUG,   tag, format, ##__VA_ARGS__)
#define LOGV( tag, format, ... ) LOG_LEVEL_RUNTIME(LOG_VERBOSE, tag, format, ##__VA_ARGS__)

#define LOG_LEVEL_RUNTIME(level, tag, format, ...) do {                  \
        if ( LOG_LEVEL_MODULE >= level && g_runtime_log_level >= level ) \
        { LOG_LEVEL(level, tag, format, ##__VA_ARGS__); }                \
    } while(0)

#ifdef __cplusplus
extern "C" {
#endif

extern int g_runtime_log_level;

#ifdef __cplusplus
}
#endif

#endif  // LOGLEVELS_H
