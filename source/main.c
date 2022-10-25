/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board_ff4.h"
#include "dsp_support.h"


/* Project includes */
#include "app.h"
#include "shell_recv.h"
#include "ble_uart_send.h"
#include "ble_uart_recv.h"
#include "utils.h"
#include "led.h"
#include "dhara_utils.h"
#include "fatfs_utils.h"
#include "syscalls.h"   // for shell/shell.c and printf()
#include "interpreter.h"
#include "eeg_reader.h"
#include "eeg_processor.h"
#include "audio.h"
#include "wavbuf.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

static const char *TAG = "main";  // Logging prefix for this module

#if (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))
#define DATA_LOG_TASK_PRIORITY 2 // was 2
#define DATA_LOG_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*(/*6+30*/ 7 )) // 6
StackType_t data_log_task_array[ DATA_LOG_TASK_STACK_SIZE];
StaticTask_t data_log_task_struct;
#endif // (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))

#define BLE_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*5) // 5
#define BLE_TASK_PRIORITY 1
StackType_t ble_task_array[ BLE_TASK_STACK_SIZE ];
StaticTask_t ble_task_struct;

#define BLE_UART_RECV_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*5) // 6
#define BLE_UART_RECV_TASK_PRIORITY 1
StackType_t ble_uart_recv_task_array[ BLE_UART_RECV_TASK_STACK_SIZE ];
StaticTask_t ble_uart_recv_task_struct;

#if (defined(ENABLE_BLE_UART_SEND_TASK) && (ENABLE_BLE_UART_SEND_TASK > 0U))
#define BLE_UART_SEND_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*4) //6
#define BLE_UART_SEND_TASK_PRIORITY 1
StackType_t ble_uart_send_task_array[ BLE_UART_SEND_TASK_STACK_SIZE ];
StaticTask_t ble_uart_send_task_struct;
#endif

#define LED_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*2)
#define LED_TASK_PRIORITY 1
StackType_t led_task_array[ LED_TASK_STACK_SIZE ];
StaticTask_t led_task_struct;

#define EEG_READER_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*3) //6
#define EEG_READER_TASK_PRIORITY 4 // used to be 5
StackType_t eeg_reader_task_array[ EEG_READER_TASK_STACK_SIZE ];
StaticTask_t eeg_reader_task_struct;

#define EEG_PROCESSOR_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*(5+10)) //5 orig, added 110 for compression algo using float (not double)
#define EEG_PROCESSOR_TASK_PRIORITY 3
StackType_t eeg_processor_task_array[ EEG_PROCESSOR_TASK_STACK_SIZE ];
StaticTask_t eeg_processor_task_struct;

#define WAVBUF_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*6) // 8
#define WAVBUF_TASK_PRIORITY 3 // used to be 4
StackType_t wavbuf_task_array[ WAVBUF_TASK_STACK_SIZE ];
StaticTask_t wavbuf_task_struct;

#define AUDIO_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*4) // 5
#define AUDIO_TASK_PRIORITY 3 // used to be 4
StackType_t audio_task_array[ AUDIO_TASK_STACK_SIZE ];
StaticTask_t audio_task_struct;

#if (defined(ENABLE_INTERPRETER_TASK) && (ENABLE_INTERPRETER_TASK > 0U))
#define INTERPRETER_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*6) // 8
#define INTERPRETER_TASK_PRIORITY 1
StackType_t interpreter_task_array[ INTERPRETER_TASK_STACK_SIZE ];
StaticTask_t interpreter_task_struct;
#endif // (defined(ENABLE_INTREPTER_TASK) && (ENABLE_INTREPTER_TASK > 0U))

#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
#define FATFS_WRITER_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*6)
#define FATFS_WRITER_TASK_PRIORITY 2
StackType_t fatfs_writer_task_array[ FATFS_WRITER_TASK_STACK_SIZE ];
StaticTask_t fatfs_writer_task_struct;
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))

#define SHELL_RECV_TASK_STACK_SIZE                (configMINIMAL_STACK_SIZE*8) // 8
#define SHELL_RECV_TASK_PRIORITY 1
StackType_t shell_recv_task_array[ SHELL_RECV_TASK_STACK_SIZE ];
StaticTask_t shell_recv_task_struct;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void system_boot_up(void);
static void dsp_boot_up(void);

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
#include "main.h"
#include "rt_nonfinite.h"
#include "sleepstagescorer.h"
#include "sleepstagescorer_terminate.h"
#include "fatfs_writer.h"
#include "loglevels.h"
#include <stdio.h>
#include "fsl_iopctl.h"
#include "fsl_dsp.h"
#include "nand_test.h"

static void system_boot_up(void)
{
	// Make sure that GPIO peripherals reset before gpios initialize
	RESET_PeripheralReset(kHSGPIO0_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kHSGPIO1_RST_SHIFT_RSTn);
	RESET_PeripheralReset(kHSGPIO2_RST_SHIFT_RSTn);

	// Init board hardware
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	BOARD_InitDebugConsole();

	//BOARD_DSP_Init();
}

int main(void)
{
	// Boot up MCU
	system_boot_up();

	syscalls_pretask_init();  // for printf()'s _write() and getchar()'s _read()

	print_version();

	// Initialize NAND SPI driver
	nand_init();
	// Initialize dhara flash translation layer
	dhara_pretask_init();
	// Filesystem init
	mount_fatfs_drive_and_format_if_needed();

	// Initialize RTOS tasks
	TaskHandle_t task_handle;

#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  LOGV(TAG, "Launching fatfs_writer task...");
  fatfs_writer_pretask_init();
  task_handle = xTaskCreateStatic(&fatfs_writer_task,
      "fatfs_writer", FATFS_WRITER_TASK_STACK_SIZE, NULL, FATFS_WRITER_TASK_PRIORITY, fatfs_writer_task_array, &fatfs_writer_task_struct);
  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)FATFS_WRITER_TASK_STACK_SIZE );
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))

#if (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))
  LOGV(TAG, "Launching data log task...");
  data_log_pretask_init();
  task_handle = xTaskCreateStatic(&data_log_task,
      "data_log", DATA_LOG_TASK_STACK_SIZE, NULL, DATA_LOG_TASK_PRIORITY, data_log_task_array, &data_log_task_struct);
  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)DATA_LOG_TASK_STACK_SIZE );
#endif

#if 0
	/* BLE tasks */
	LOGV(TAG, "Launching BLE task...");
	ble_pretask_init();
	task_handle = xTaskCreateStatic(&ble_task,
			"ble", BLE_TASK_STACK_SIZE, NULL, BLE_TASK_PRIORITY, ble_task_array, &ble_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *) BLE_TASK_STACK_SIZE );

	LOGV(TAG, "Launching BLE uart_recv task...");
	ble_uart_recv_pretask_init();
	task_handle = xTaskCreateStatic(&ble_uart_recv_task,
	      "ble_uart_recv", BLE_UART_RECV_TASK_STACK_SIZE, NULL, BLE_UART_RECV_TASK_PRIORITY, ble_uart_recv_task_array, &ble_uart_recv_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *) BLE_UART_RECV_TASK_STACK_SIZE );

	/* LED task */
	LOGV(TAG, "Launching led task...");
	led_pretask_init();
	task_handle = xTaskCreateStatic(&led_task,
	  "led", LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, led_task_array, &led_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)LED_TASK_STACK_SIZE );

#endif

	/* EEG tasks */
	LOGV(TAG, "Launching eeg_reader task...");
	eeg_reader_pretask_init();
	task_handle = xTaskCreateStatic(&eeg_reader_task,
	  "eeg_reader", EEG_READER_TASK_STACK_SIZE, NULL, EEG_READER_TASK_PRIORITY, eeg_reader_task_array, &eeg_reader_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)EEG_READER_TASK_STACK_SIZE );

	LOGV(TAG, "Launching eeg_processor task...");
	eeg_processor_pretask_init();
	task_handle = xTaskCreateStatic(&eeg_processor_task,
	  "eeg_processor", EEG_PROCESSOR_TASK_STACK_SIZE, NULL, EEG_PROCESSOR_TASK_PRIORITY, eeg_processor_task_array, &eeg_processor_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)EEG_PROCESSOR_TASK_STACK_SIZE );

	/* Audio tasks */
	LOGV(TAG, "Launching audio task...");
	audio_pretask_init();
	task_handle = xTaskCreateStatic(&audio_task,
	  "audio", AUDIO_TASK_STACK_SIZE, NULL, AUDIO_TASK_PRIORITY, audio_task_array, &audio_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)AUDIO_TASK_STACK_SIZE );

	LOGV(TAG, "Launching wavbuf task...");
	wavbuf_pretask_init();
	task_handle = xTaskCreateStatic(&wavbuf_task,
	  "wavbuf", WAVBUF_TASK_STACK_SIZE, NULL, WAVBUF_TASK_PRIORITY, wavbuf_task_array, &wavbuf_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)WAVBUF_TASK_STACK_SIZE );

	/* Interpreter task */
#if (defined(ENABLE_INTERPRETER_TASK) && (ENABLE_INTERPRETER_TASK > 0U))
    LOGV(TAG, "Launching interpreter task...");
    interpreter_pretask_init();
    task_handle = xTaskCreateStatic(&interpreter_task,
      "interpreter", INTERPRETER_TASK_STACK_SIZE, NULL, INTERPRETER_TASK_PRIORITY, interpreter_task_array, &interpreter_task_struct);
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)INTERPRETER_TASK_STACK_SIZE );
#endif

	/* Shell task */
	LOGV(TAG, "Launching shell recv task...");
	shell_recv_pretask_init();
	task_handle = xTaskCreateStatic(&shell_recv_task,
	  "shell_recv", SHELL_RECV_TASK_STACK_SIZE, NULL, SHELL_RECV_TASK_PRIORITY, shell_recv_task_array, &shell_recv_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)SHELL_RECV_TASK_STACK_SIZE );

	//DSP_Start();

	vTaskStartScheduler();

	for (;;);
}
