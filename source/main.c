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
#include "fatfs_writer.h"
#include "loglevels.h"
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
	int x = 0;

	while(1)
	{
		x++;
		x--;
	}



	return 0;
}
