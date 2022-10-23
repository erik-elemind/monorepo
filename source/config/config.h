#ifndef CONFIG_H
#define CONFIG_H

#include "version.h"
#include "../config/board_config.h"
#include "config_tracealyzer.h"
#include "config_tracealyzer_isr.h"

#define BUILD_CONFIG_SLEEP_STUDY_3  (1U)
#define BUILD_CONFIG_BETA_1 (2U)

// Choose the build configuration from the list above
#define BUILD_CONFIG BUILD_CONFIG_BETA_1


// For UART terminal output messages:
/*
    LOG_NONE (0)
   LOG_ERROR (1)
    LOG_WARN (2)
    LOG_INFO (3)   // LOG_INFO (3) recommended for normal use
   LOG_DEBUG (4)
 LOG_VERBOSE (5)
*/
#define DEFAULT_LOG_LEVEL LOG_VERBOSE


//
// Shell Config
//

// Use a FreeRTOS Task (vs. a bare-metal tick once per loop):
#define CONFIG_SHELL_FREERTOS 1

// SHELL_WELCOME_STRING
#define SHELL_WELCOME_STRING "\n**** Morpheus FW ****\n\n"\
" Type 'help' or '?' to see a list of shell commands.\n\n"

// If using semihosting under Quick Settings > Set library/header type
// linking will fail because we redefine _write()/_read() in syscalls.c.
// By setting this define, we avoid the inclusion of those conflicting functions.
//#define CONFIG_USE_SEMIHOSTING 1

// Shell output device (only define one)
#define CONFIG_SHELL_UART

// Enable the System Watchdog Timer
// 0U - disable the timer
// 1U - enable the timer
// (Used in system_monitor.h)
#define ENABLE_SYSTEM_WATCHDOG (0U)

// Monitor the battery charger temperature sensor.
// 0U - ignore temp sensor.
// 1U - monitor temp sensor. (production default)
// (Used in battery_charger.c)
#define ENABLE_CHARGER_TEMP_SENSOR (1U)

// Enable USB plug/unplug events
// 0U - ignore USB plug/unplug events
// 1U - use USB plug/unplug events (production default)
// (Used in app.c)
#define ENABLE_CHARGER_PLUGGED_EVENTS (1U)

// Enable timer for auto power-off of core processor
// 0U - disable timer-based auto power-off
// 1U - enable timer-based auto power-off (production default)
// (Used in app.c)
#define ENABLE_APP_POWER_OFF_TIMER (0U)

// Enable timer for auto power-off of BLE
// 0U - disable timer-based auto power-off
// 1U - enable timer-based auto power-off (production default)
#define ENABLE_BLE_POWER_OFF_TIMER (0U)

// Enable the BLE_UART_SEND task, affects how ble_uart_send_buf() operates.
// 0U - disable the task, ble_uart_send_buf() calls USART_RTOS_Send
// 1U - enable the task, ble_uart_send_buf() pushes messages onto stream buf
#define ENABLE_BLE_UART_SEND_TASK (0U)

#define ENABLE_APP_TASK (0U) // TODO: Reenable when app task is thoughtfully added
#define ENABLE_AUDIO_TASK (1U)
#define ENABLE_AUDIO_MP3_TASK (0U) //(0U)
#define ENABLE_DATA_LOG_TASK (0U) // TODO: Reenable when data log task is thoughtfully added
#define ENABLE_EEG_READER_TASK (1U)
#define ENABLE_EEG_PROCESSOR_TASK (1U)
#define ENABLE_HRM_TASK (1U)
#define ENABLE_ACCEL_TASK (1U)


#define ENABLE_WAVBUF_TASK (1U)

#define USE_EEG_INTERRUPT_INITIATED_DMA (0U)

// Config Streaming Log Data (used in data_log.c)
// 0U - stream NO logged data
// 1U - stream ALL logged data
// 2U - stream ONLY sample numbers
#ifndef CONFIG_STREAMING_LOG_DATA
#define CONFIG_STREAMING_LOG_DATA 1U
#endif

#define ENABLE_FS_WRITER (1U)

#define ENABLE_USB_DEBUGGING (0U)

// Enable PowerQuad interrupt
// This settings selects how the program waits for the PowerQuad to complete.
// 0U - use event trigger and spin lock (default SDK example)
// 1U - use interrupt
// (Used in powerquad_helper.c)
// Note: In practice, with an FFT size of 128, it doesn't make a difference whether event or interrupt is used.
#define ENABLE_POWERQUAD_INTERRUPT (0U)

// Enable NO COPY wav buffer
#define ENABLE_NO_COPY_WAV_BUFFER (1U)

#define ENABLE_OFFLINE_EEG_COMPRESSION (0U)

// Enable different solutions to allow USB and XTAL32K CLOCK TO RUN
// 0U - do nothing (error)
// 1U - 12 sec delay before scheduler starts, external 32kHz
// 2U - 12 sec delay after scheduler starts, external 32kHz
// 3U - standard delay, use internal 32kHz
#define USBC_XTAL32K_COMPATBILITY_OPTION (3U)


#if (defined(BUILD_CONFIG) && BUILD_CONFIG==BUILD_CONFIG_SLEEP_STUDY_3)
  #define CONFIG_DATALOG_USE_TIME_STRING (1U)
#elif (defined(BUILD_CONFIG) && BUILD_CONFIG==BUILD_CONFIG_BETA_1)
  #define CONFIG_DATALOG_USE_TIME_STRING (0U)
#else
  #error "Missing BUILD_CONFIG behavior for CONFIG_DATALOG_USE_TIME_STRING"
#endif

#define AUDIO_ENABLE_FG_WAV (0U)

#if (defined(BUILD_CONFIG) && BUILD_CONFIG==BUILD_CONFIG_SLEEP_STUDY_3)
  #define ENABLE_POWER_BUTTON_RESET (1U)
#elif (defined(BUILD_CONFIG) && BUILD_CONFIG==BUILD_CONFIG_BETA_1)
  #define ENABLE_POWER_BUTTON_RESET (0U)
#else
  #error "Missing BUILD_CONFIG behavior for ENABLE_POWER_BUTTON_RESET"
#endif


/*****************************************************************************/
// TEST DECLARATIONS

// Enable stress-testing for Dhara. See
// source_code/dhara_interface/dhara_ppstress.h for more details
#define ENABLE_DHARA_PPSTRESS

// Enable memory unit tests
#define ENABLE_STREAM_MEMORY_TEST_COMMANDS (0U)

// Compile the a FreeRTOS task to test for noise (i.e. periodic spi flash r/w).
// 0U - do NOT include/compile
// 1U - include/compile
#define ENABLE_NOISE_TEST (0U)

// Compile the file system test commands
// 0U - do NOT compile/link "fs_test..." functions.
// 1U - DO compiler/link "fs_test..." functions.
#define ENABLE_FS_TEST_COMMANDS (0U)

// Enable testing of double-quote parser
// 0U - Do NOT run parser test.
// 1U - Run parser test at the start of main(), before running tasks, prints to debug_uart.
#define ENABLE_PARSE_DOUBLE_QUOTE_TEST (0U)

#define ENABLE_PARSE_DOT_NOTATION_TEST (0U)

#define ENABLE_COMPRESSION_TEST_TASK (0U)

#define ENABLE_COMPRESSION_TEST (0U)

#endif  // CONFIG_H
