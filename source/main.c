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
#include "ble_uart_send.h"
#include "ble_uart_recv.h"
#include "led.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

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


static void system_boot_up(void)
{
	// Init board hardware
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	BOARD_InitDebugConsole();

	BOARD_DSP_Init();
}

int main(void)
{
	// Boot up MCU
	system_boot_up();
	print_version();

	// Initialize RTOS tasks
#if 1
	TaskHandle_t task_handle;
#endif

	/* BLE tasks */
	printf("Launching BLE task...\n\r");
	ble_pretask_init();
	task_handle = xTaskCreateStatic(&ble_task,
			"ble", BLE_TASK_STACK_SIZE, NULL, BLE_TASK_PRIORITY, ble_task_array, &ble_task_struct);

	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *) BLE_TASK_STACK_SIZE );

	printf("Launching BLE uart_recv task...\n\r");
	ble_uart_recv_pretask_init();
	task_handle = xTaskCreateStatic(&ble_uart_recv_task,
	      "ble_uart_recv", BLE_UART_RECV_TASK_STACK_SIZE, NULL, BLE_UART_RECV_TASK_PRIORITY, ble_uart_recv_task_array, &ble_uart_recv_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *) BLE_UART_RECV_TASK_STACK_SIZE );

	#if (defined(ENABLE_BLE_UART_SEND_TASK) && (ENABLE_BLE_UART_SEND_TASK > 0U))
	printf("Launching BLE uart_send task...\n\r");
	ble_uart_send_pretask_init();
	task_handle = xTaskCreateStatic(&ble_uart_send_task,
		  "ble_uart_send", BLE_UART_SEND_TASK_STACK_SIZE, NULL, BLE_UART_SEND_TASK_PRIORITY, ble_uart_send_task_array, &ble_uart_send_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)BLE_UART_SEND_TASK_STACK_SIZE );
	#endif

	printf("Launching led task...\n\r");
	led_pretask_init();
	task_handle = xTaskCreateStatic(&led_task,
	  "led", LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, led_task_array, &led_task_struct);
	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)LED_TASK_STACK_SIZE );

	DSP_Start();

	vTaskStartScheduler();

	for (;;); // loop to allow new debug session to connect
}
