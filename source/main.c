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
#include "board.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

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
#include <stdio.h>

static const char *TAG = "main";  // Logging prefix for this module

static void system_boot_up(void);

static void system_boot_up(void)
{
	// Init board hardware
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitDebugConsole();

}



int main(void)
{
	TaskHandle_t task_handle;

	// Boot up MCU
	system_boot_up();

	//Fat FS Write Task
	LOGV(TAG, "Launching fatfs_writer task...");

	fatfs_writer_pretask_init();

	task_handle = xTaskCreateStatic(&fatfs_writer_task,
		"fatfs_writer",
		FATFS_WRITER_TASK_STACK_SIZE,
		NULL,
		FATFS_WRITER_TASK_PRIORITY,
		fatfs_writer_task_array,
		&fatfs_writer_task_struct);

	vTaskSetThreadLocalStoragePointer( task_handle, 0, (void *)FATFS_WRITER_TASK_STACK_SIZE );


	return 0;
}
