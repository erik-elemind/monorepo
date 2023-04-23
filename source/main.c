/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_powerquad.h"


/* Project includes */
#include "config.h"
#include "micro_clock.h"
#include "loglevels.h"
#include "dsp_support.h"
#include "app.h"
#include "data_log.h"
#include "button.h"
#include "button_config.h"
#include "system_monitor.h"
#include "shell_recv.h"
#include "ble_uart_send.h"
#include "ble_uart_recv.h"
#include "ble_shell.h"
#include "utils.h"
#include "led.h"
#include "audio.h"
#include "audio_stream_task.h"
#include "wavbuf.h"
#include "hrm.h"
#include "accel.h"
#include "fatfs_writer.h"  // task to handle slow writes
#include "nand.h"
#include "dhara_utils.h"
#include "fatfs_utils.h"
#include "syscalls.h"   // for shell/shell.c and printf()
#include "interpreter.h"
#include "eeg_reader.h"
#include "eeg_processor.h"
#include "erp.h"
#include "ml.h"
#include "user_metrics.h"
#include "pmic_pca9420.h"

#include "powerquad_helper.h"

#include "fsl_usart_rtos_additional.h"


#include "main.h"
#include "rt_nonfinite.h"
#include "sleepstagescorer.h"
#include "sleepstagescorer_terminate.h"
#include <stdio.h>
#include "fsl_iopctl.h"
#include "fsl_dsp.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

static const char *TAG = "main";  // Logging prefix for this module

/* BE CAUTIOUS TO SET CORRECT VOLTAGE RANGE ACCORDING TO YOUR BOARD/APPLICATION. PAD SUPPLY BEYOND THE RANGE DO
      HARM TO THE SILICON. */
power_pad_vrange_t vrange = {.Vdde0Range = kPadVol_171_198,
							.Vdde1Range = kPadVol_171_198,
							/* SD0 voltage is switchable, but in power_manager demo, it's fixed 3.3V. */
							.Vdde2Range = kPadVol_171_198};

//
// Set task stack sizes and priorities:
//

#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
#define FATFS_WRITER_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*6)
#define FATFS_WRITER_TASK_PRIORITY 2
StackType_t fatfs_writer_task_array[ FATFS_WRITER_TASK_STACK_SIZE ];
StaticTask_t fatfs_writer_task_struct;
#endif

#if (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))
#define DATA_LOG_TASK_PRIORITY 2 // was 2
#define DATA_LOG_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*(/*6+30*/ 7 )) // 6
StackType_t data_log_task_array[ DATA_LOG_TASK_STACK_SIZE];
StaticTask_t data_log_task_struct;
#endif

#if (defined(ENABLE_BUTTON_TASK) && (ENABLE_BUTTON_TASK > 0U))
#define BUTTON_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*2)
#define BUTTON_TASK_PRIORITY 1
StackType_t button_task_array[ BUTTON_TASK_STACK_SIZE ];
StaticTask_t button_task_struct;
#endif

#if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))
#define AUDIO_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*4) // 5
#define AUDIO_TASK_PRIORITY 1
StackType_t audio_task_array[ AUDIO_TASK_STACK_SIZE ];
StaticTask_t audio_task_struct;

#define AUDIO_STREAM_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*4) // 5
#define AUDIO_STREAM_TASK_PRIORITY 3 // used to be 4
StackType_t audio_stream_task_array[ AUDIO_STREAM_TASK_STACK_SIZE ];
StaticTask_t audio_stream_task_struct;
#endif

#if (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK > 0U))
#define WAVBUF_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*6) // 8
#define WAVBUF_TASK_PRIORITY 3 // used to be 4
StackType_t wavbuf_task_array[ WAVBUF_TASK_STACK_SIZE ];
StaticTask_t wavbuf_task_struct;
#endif

#if (defined(ENABLE_MP3_TASK) && (ENABLE_MP3_TASK > 0U))
// ToDo: Add MP3 Task definitions
#endif

#if (defined(ENABLE_EEG_READER_TASK) && (ENABLE_EEG_READER_TASK > 0U))
#define EEG_READER_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*3) //6
#define EEG_READER_TASK_PRIORITY 4 // used to be 5
StackType_t eeg_reader_task_array[ EEG_READER_TASK_STACK_SIZE ];
StaticTask_t eeg_reader_task_struct;
#endif

#if (defined(ENABLE_EEG_PROCESSOR_TASK) && (ENABLE_EEG_PROCESSOR_TASK > 0U))
#define EEG_PROCESSOR_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*(5+10)) //5 orig, added 110 for compression algo using float (not double)
#define EEG_PROCESSOR_TASK_PRIORITY 3
StackType_t eeg_processor_task_array[ EEG_PROCESSOR_TASK_STACK_SIZE ];
StaticTask_t eeg_processor_task_struct;
#endif

#if (defined(ENABLE_HRM_TASK) && (ENABLE_HRM_TASK > 0U))
#define HRM_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*3)
#define HRM_TASK_PRIORITY 2
StackType_t hrm_task_array[ HRM_TASK_STACK_SIZE ];
StaticTask_t hrm_task_struct;
#endif

#if (defined(ENABLE_ACCEL_TASK) && (ENABLE_ACCEL_TASK > 0U))
#define ACCEL_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*2)
#define ACCEL_TASK_PRIORITY 2
StackType_t accel_task_array[ ACCEL_TASK_STACK_SIZE ];
StaticTask_t accel_task_struct;
#endif

#if (defined(ENABLE_SYSTEM_MONITOR_TASK) && (ENABLE_SYSTEM_MONITOR_TASK > 0U))
#define SYSTEM_MONITOR_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*4)
#define SYSTEM_MONITOR_TASK_PRIORITY 2
StackType_t system_monitor_task_array[ SYSTEM_MONITOR_TASK_STACK_SIZE ];
StaticTask_t system_monitor_task_struct;
#endif

#if (defined(ENABLE_INTERPRETER_TASK) && (ENABLE_INTERPRETER_TASK > 0U))
#define INTERPRETER_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE*6) // 8
#define INTERPRETER_TASK_PRIORITY 1
StackType_t interpreter_task_array[ INTERPRETER_TASK_STACK_SIZE ];
StaticTask_t interpreter_task_struct;
#endif

#if (defined(ENABLE_BLE_TASK) && (ENABLE_BLE_TASK > 0U))
#define BLE_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*5) // 5
#define BLE_TASK_PRIORITY 1
StackType_t ble_task_array[ BLE_TASK_STACK_SIZE ];
StaticTask_t ble_task_struct;
#endif

#if (defined(ENABLE_BLE_UART_RECV_TASK) && (ENABLE_BLE_UART_RECV_TASK > 0U))
#define BLE_UART_RECV_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*5) // 6
#define BLE_UART_RECV_TASK_PRIORITY 1
StackType_t ble_uart_recv_task_array[ BLE_UART_RECV_TASK_STACK_SIZE ];
StaticTask_t ble_uart_recv_task_struct;
#endif

#if (defined(ENABLE_BLE_UART_SEND_TASK) && (ENABLE_BLE_UART_SEND_TASK > 0U))
#define BLE_UART_SEND_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*4) //6
#define BLE_UART_SEND_TASK_PRIORITY 1
StackType_t ble_uart_send_task_array[ BLE_UART_SEND_TASK_STACK_SIZE ];
StaticTask_t ble_uart_send_task_struct;
#endif

#if (defined(ENABLE_BLE_SHELL_TASK) && (ENABLE_BLE_SHELL_TASK > 0U))
#define BLE_SHELL_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE*8) // 8
#define BLE_SHELL_TASK_PRIORITY 1
StackType_t ble_shell_task_array[ BLE_SHELL_TASK_STACK_SIZE ];
StaticTask_t ble_shell_task_struct;
#endif

#if (defined(ENABLE_SHELL_RECV_TASK) && (ENABLE_SHELL_RECV_TASK > 0U))
#define SHELL_RECV_TASK_STACK_SIZE                (configMINIMAL_STACK_SIZE*8) // 8
#define SHELL_RECV_TASK_PRIORITY 1
StackType_t shell_recv_task_array[ SHELL_RECV_TASK_STACK_SIZE ];
StaticTask_t shell_recv_task_struct;
#endif

#if (defined(ENABLE_ERP_TASK) && (ENABLE_ERP_TASK > 0U))
#define ERP_TASK_STACK_SIZE                (configMINIMAL_STACK_SIZE*8) // 8
#define ERP_TASK_PRIORITY 1
StackType_t erp_task_array[ ERP_TASK_STACK_SIZE ];
StaticTask_t erp_task_struct;
#endif

#if (defined(ENABLE_APP_TASK) && (ENABLE_APP_TASK > 0U))
#define APP_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*3)
#define APP_TASK_PRIORITY 1
StackType_t app_task_array[ APP_TASK_STACK_SIZE ];
StaticTask_t app_task_struct;
#endif

#if (defined(ENABLE_ML_TASK) && (ENABLE_ML_TASK > 0U))
#define ML_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*100) //TODO: change to more appropriate value
#define ML_TASK_PRIORITY 2
StackType_t ml_task_array[ ML_TASK_STACK_SIZE ];
StaticTask_t ml_task_struct;
#endif

#if (defined(ENABLE_USER_METRICS_TASK) && (ENABLE_USER_METRICS_TASK > 0U))
#define USER_METRICS_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE*5) //TODO: change to more appropriate value
#define USER_METRICS_TASK_PRIORITY 3
StackType_t user_metrics_task_array[ USER_METRICS_TASK_STACK_SIZE ];
StaticTask_t user_metrics_task_struct;
#endif

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
	// Enable flow control in BLE UART, which is not natively supported by the USART FreeRTOS Driver.
	// This line of code overrides the behavior of the auto-generated code in BOARD_InitBootPeripherals().
	USART_RTOS_Init_FlowControl(&FC0_BLE_UART_rtos_handle, &FC0_BLE_UART_usart_handle, &FC0_BLE_UART_rtos_config, true);

	BOARD_InitBLE();
	BOARD_InitDebugConsole();

	led_init();

	pmic_init();

	// Enable USB, buttons, and BLE UART for DeepSleepIRQs
	POWER_UpdatePmicRecoveryTime(1);
	POWER_SetPadVolRange(&vrange);
	EnableDeepSleepIRQ(PIN_INT5_IRQn); //usr_but1
	EnableDeepSleepIRQ(PIN_INT6_IRQn); //usr_but2
	EnableDeepSleepIRQ(PIN_INT7_IRQn); //power_but
	EnableDeepSleepIRQ(RTC_IRQn); // rtc
	EnableDeepSleepIRQ(FLEXCOMM0_IRQn); // BLE usart
	EnableDeepSleepIRQ(FLEXCOMM5_IRQn); // debug usart

#if DSP_IMAGE_COPY_TO_RAM
	BOARD_DSP_Init();
#endif

    // Enable PowerQuad - Needed for ecHT computation in eeg_processor.h
    pqhelper_init();

    // init micro clock
    init_micro_clock(); // TODO: Fix this clock to compensate for the 16Mhz clock input.

}

int main(void)
{
#if (defined(ENABLE_TRACEALYZER) && (ENABLE_TRACEALYZER > 0U))
  /* Init Percepio Tracealyzer */
//  vTraceEnable(TRC_START);
  vTraceEnable(TRC_INIT);
  init_tracealyzer_isr();
#endif

	// Boot up MCU
	system_boot_up();

	syscalls_pretask_init();  // for printf()'s _write() and getchar()'s _read()

	// Do this as soon as the uart is working, in case something stalls
	print_version();

	// Initialize NAND SPI driver
	nand_init();
	// Initialize dhara flash translation layer
	dhara_pretask_init();
	// Filesystem init
	mount_fatfs_drive_and_format_if_needed();

    //
    // Launch all Tasks:
    //

	// Initialize RTOS tasks
	TaskHandle_t task_handle;

    /* File System Writer task */
#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  LOGV(TAG, "Launching fatfs_writer task...");
  fatfs_writer_pretask_init();
  task_handle = xTaskCreateStatic(&fatfs_writer_task,
      "fatfs_writer", FATFS_WRITER_TASK_STACK_SIZE, NULL, FATFS_WRITER_TASK_PRIORITY, fatfs_writer_task_array, &fatfs_writer_task_struct);
  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)FATFS_WRITER_TASK_STACK_SIZE );
#endif

    /* Data Log task */
#if (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))
  LOGV(TAG, "Launching data log task...");
  data_log_pretask_init();
  task_handle = xTaskCreateStatic(&data_log_task,
      "data_log", DATA_LOG_TASK_STACK_SIZE, NULL, DATA_LOG_TASK_PRIORITY, data_log_task_array, &data_log_task_struct);
  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)DATA_LOG_TASK_STACK_SIZE );
#endif

	/* Button task */
#if (defined(ENABLE_BUTTON_TASK) && (ENABLE_BUTTON_TASK > 0U))
	  LOGV(TAG, "Launching button task...");
	  // Initialize Buttons
	  button_params_t* button_params = button_pretask_init();
	  task_handle = xTaskCreateStatic(&button_task,
	      "button", BUTTON_TASK_STACK_SIZE, button_params, BUTTON_TASK_PRIORITY, button_task_array, &button_task_struct);
	  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)BUTTON_TASK_STACK_SIZE );
#endif

	/* Audio tasks */
#if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))
	LOGV(TAG, "Launching audio task...");
	audio_pretask_init();
	task_handle = xTaskCreateStatic(&audio_task,
	  "audio", AUDIO_TASK_STACK_SIZE, NULL, AUDIO_TASK_PRIORITY, audio_task_array, &audio_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)AUDIO_TASK_STACK_SIZE );

	LOGV(TAG, "Launching audio compute task...");
	audio_stream_pretask_init();
	task_handle = xTaskCreateStatic(&audio_stream_task,
	  "audio", AUDIO_STREAM_TASK_STACK_SIZE, NULL, AUDIO_STREAM_TASK_PRIORITY, audio_stream_task_array, &audio_stream_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)AUDIO_STREAM_TASK_STACK_SIZE );
#endif

#if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))
#if (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK > 0U))
	LOGV(TAG, "Launching wavbuf task...");
	wavbuf_pretask_init();
	task_handle = xTaskCreateStatic(&wavbuf_task,
	  "wavbuf", WAVBUF_TASK_STACK_SIZE, NULL, WAVBUF_TASK_PRIORITY, wavbuf_task_array, &wavbuf_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)WAVBUF_TASK_STACK_SIZE );
#endif
#endif

#if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))
#if (defined(ENABLE_AUDIO_MP3_TASK) && (ENABLE_AUDIO_MP3_TASK > 0U))
	// ToDo: Add MP3 Task here
#endif
#endif

	/* EEG tasks */
#if (defined(ENABLE_EEG_READER_TASK) && (ENABLE_EEG_READER_TASK > 0U))
	LOGV(TAG, "Launching eeg_reader task...");
	eeg_reader_pretask_init();
	task_handle = xTaskCreateStatic(&eeg_reader_task,
	  "eeg_reader", EEG_READER_TASK_STACK_SIZE, NULL, EEG_READER_TASK_PRIORITY, eeg_reader_task_array, &eeg_reader_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)EEG_READER_TASK_STACK_SIZE );
#endif

#if (defined(ENABLE_EEG_PROCESSOR_TASK) && (ENABLE_EEG_PROCESSOR_TASK > 0U))
	LOGV(TAG, "Launching eeg_processor task...");
	eeg_processor_pretask_init();
	task_handle = xTaskCreateStatic(&eeg_processor_task,
	  "eeg_processor", EEG_PROCESSOR_TASK_STACK_SIZE, NULL, EEG_PROCESSOR_TASK_PRIORITY, eeg_processor_task_array, &eeg_processor_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)EEG_PROCESSOR_TASK_STACK_SIZE );
#endif

	/* Heart Rate Monitor Task */
#if (defined(ENABLE_HRM_TASK) && (ENABLE_HRM_TASK > 0U))
  LOGV(TAG, "Launching hrm task...");
  hrm_pretask_init();
  task_handle = xTaskCreateStatic(&hrm_task,
      "hrm", HRM_TASK_STACK_SIZE, NULL, HRM_TASK_PRIORITY, hrm_task_array, &hrm_task_struct);
  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)HRM_TASK_STACK_SIZE );
#endif

  /* Accelerometer Task */
#if (defined(ENABLE_ACCEL_TASK) && (ENABLE_ACCEL_TASK > 0U))
  LOGV(TAG, "Launching accel task...");
  accel_pretask_init(&ACCEL_I2C_RTOS_HANDLE);
  task_handle = xTaskCreateStatic(&accel_task,
      "accel", ACCEL_TASK_STACK_SIZE, NULL, ACCEL_TASK_PRIORITY, accel_task_array, &accel_task_struct);
  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)ACCEL_TASK_STACK_SIZE );
#endif

	/* System Monitor task */
#if (defined(ENABLE_SYSTEM_MONITOR_TASK) && (ENABLE_SYSTEM_MONITOR_TASK > 0U))
    LOGV(TAG, "Launching system_monitor task...");
    system_monitor_pretask_init();
    task_handle = xTaskCreateStatic(&system_monitor_task,
        "system_monitor", SYSTEM_MONITOR_TASK_STACK_SIZE, NULL, SYSTEM_MONITOR_TASK_PRIORITY, system_monitor_task_array, &system_monitor_task_struct);
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)SYSTEM_MONITOR_TASK_STACK_SIZE );
#endif

	/* Interpreter task */
#if (defined(ENABLE_INTERPRETER_TASK) && (ENABLE_INTERPRETER_TASK > 0U))
    LOGV(TAG, "Launching interpreter task...");
    interpreter_pretask_init();
    task_handle = xTaskCreateStatic(&interpreter_task,
      "interpreter", INTERPRETER_TASK_STACK_SIZE, NULL, INTERPRETER_TASK_PRIORITY, interpreter_task_array, &interpreter_task_struct);
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)INTERPRETER_TASK_STACK_SIZE );
#endif

    /* BLE tasks */
#if (defined(ENABLE_BLE_TASK) && (ENABLE_BLE_TASK > 0U))
	LOGV(TAG, "Launching BLE task...");
	ble_pretask_init();
	task_handle = xTaskCreateStatic(&ble_task,
			"ble", BLE_TASK_STACK_SIZE, NULL, BLE_TASK_PRIORITY, ble_task_array, &ble_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *) BLE_TASK_STACK_SIZE );
#endif

#if (defined(ENABLE_BLE_TASK) && (ENABLE_BLE_TASK > 0U))
#if (defined(ENABLE_BLE_UART_RECV_TASK) && (ENABLE_BLE_UART_RECV_TASK > 0U))
	LOGV(TAG, "Launching BLE uart_recv task...");
	ble_uart_recv_pretask_init();
	task_handle = xTaskCreateStatic(&ble_uart_recv_task,
	      "ble_uart_recv", BLE_UART_RECV_TASK_STACK_SIZE, NULL, BLE_UART_RECV_TASK_PRIORITY, ble_uart_recv_task_array, &ble_uart_recv_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *) BLE_UART_RECV_TASK_STACK_SIZE );
#endif
#endif

#if (defined(ENABLE_BLE_TASK) && (ENABLE_BLE_TASK > 0U))
#if (defined(ENABLE_BLE_SHELL_TASK) && (ENABLE_BLE_SHELL_TASK > 0U))
	  LOGV(TAG, "Launching BLE shell task...");
	  ble_shell_pretask_init();
	  task_handle = xTaskCreateStatic(&ble_shell_task,
	      "ble_shell", BLE_SHELL_TASK_STACK_SIZE, NULL, BLE_SHELL_TASK_PRIORITY, ble_shell_task_array, &ble_shell_task_struct);
	  vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)BLE_SHELL_TASK_STACK_SIZE );
#endif
#endif

	/* Shell task */
#if (defined(ENABLE_SHELL_RECV_TASK) && (ENABLE_SHELL_RECV_TASK > 0U))
	LOGV(TAG, "Launching shell recv task...");
	shell_recv_pretask_init();
	task_handle = xTaskCreateStatic(&shell_recv_task,
	  "shell_recv", SHELL_RECV_TASK_STACK_SIZE, NULL, SHELL_RECV_TASK_PRIORITY, shell_recv_task_array, &shell_recv_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)SHELL_RECV_TASK_STACK_SIZE );
#endif

	/* ERP task */
#if (defined(ENABLE_ERP_TASK) && (ENABLE_ERP_TASK > 0U))
    LOGV(TAG, "Launching erp task...");
    erp_pretask_init();
    task_handle = xTaskCreateStatic(&erp_task,
      "erp", ERP_TASK_STACK_SIZE, NULL, ERP_TASK_PRIORITY, erp_task_array, &erp_task_struct);
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)ERP_TASK_STACK_SIZE );
#endif

    /* App task */
#if (defined(ENABLE_APP_TASK) && (ENABLE_APP_TASK > 0U))
    LOGV(TAG, "Launching app task...");
    app_pretask_init();
    task_handle = xTaskCreateStatic(&app_task,
        "app", APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, app_task_array, &app_task_struct);
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)APP_TASK_STACK_SIZE );
#endif

    /* ML task */
#if (defined(ENABLE_ML_TASK) && (ENABLE_ML_TASK > 0U))
    LOGV(TAG, "Launching ML task...");
    ml_pretask_init();
    task_handle = xTaskCreateStatic(&ml_task,
        "ml", ML_TASK_STACK_SIZE, NULL, ML_TASK_PRIORITY, ml_task_array, &ml_task_struct);
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)ML_TASK_STACK_SIZE );
#endif

    /* User Metrics task */
#if (defined(ENABLE_USER_METRICS_TASK) && (ENABLE_USER_METRICS_TASK > 0U))
    LOGV(TAG, "Launching user metrics task...");
    user_metrics_pretask_init();
    task_handle = xTaskCreateStatic(&user_metrics_task,
        "user_metrics", USER_METRICS_TASK_STACK_SIZE, NULL, USER_METRICS_TASK_PRIORITY, user_metrics_task_array, &user_metrics_task_struct);
    vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)USER_METRICS_TASK_STACK_SIZE );
#endif

#if DSP_IMAGE_COPY_TO_RAM
	DSP_Start();
#endif

	vTaskStartScheduler();

	for (;;);
}
