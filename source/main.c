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
static void system_boot_up(void);

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

extern const uint32_t NAND_FLEXSPI_LUT[64];

static void system_boot_up(void)
{
	// Init board hardware
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	BOARD_InitDebugConsole();
}

const uint32_t LED_PWM_ON = (/* Pin is configured as PIO0_27 */
                               IOPCTL_PIO_FUNC0 |
                               /* Enable pull-up / pull-down function */
                               IOPCTL_PIO_PUPD_EN |
                               /* Enable pull-up function */
                               IOPCTL_PIO_PULLDOWN_EN |
                               /* Disable input buffer function */
                               IOPCTL_PIO_INBUF_DI |
                               /* Normal mode */
                               IOPCTL_PIO_SLEW_RATE_NORMAL |
                               /* Normal drive */
                               IOPCTL_PIO_FULLDRIVE_DI |
                               /* Analog mux is disabled */
                               IOPCTL_PIO_ANAMUX_DI |
                               /* Pseudo Output Drain is disabled */
                               IOPCTL_PIO_PSEDRAIN_DI |
                               /* Input function is not inverted */
                               IOPCTL_PIO_INV_DI);

const uint32_t LED_PWM_OFF = (/* Pin is configured as PIO0_27 */
                               IOPCTL_PIO_FUNC0 |
                               /* Enable pull-up / pull-down function */
                               IOPCTL_PIO_PUPD_EN |
                               /* Enable pull-up function */
                               IOPCTL_PIO_PULLUP_EN |
                               /* Disable input buffer function */
                               IOPCTL_PIO_INBUF_DI |
                               /* Normal mode */
                               IOPCTL_PIO_SLEW_RATE_NORMAL |
                               /* Normal drive */
                               IOPCTL_PIO_FULLDRIVE_DI |
                               /* Analog mux is disabled */
                               IOPCTL_PIO_ANAMUX_DI |
                               /* Pseudo Output Drain is disabled */
                               IOPCTL_PIO_PSEDRAIN_DI |
                               /* Input function is not inverted */
                               IOPCTL_PIO_INV_DI);






int main(void)
{

	flexspi_device_config_t deviceconfig = {
	    .flexspiRootClk       = 1000000,
		.isSck2Enabled        = false,
	    .flashSize            = 0x80000,
	    .CSIntervalUnit       = kFLEXSPI_CsIntervalUnit1SckCycle,
	    .CSInterval           = 2,
	    .CSHoldTime           = 3,
	    .CSSetupTime          = 3,
	    .dataValidTime        = 2,
	    .columnspace          = 0,
	    .enableWordAddress    = 0,
	    .AWRSeqIndex          = 0,
	    .AWRSeqNumber         = 0,
	    .ARDSeqIndex          = 0,
	    .ARDSeqNumber         = 0,
	    .AHBWriteWaitUnit     = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
	    .AHBWriteWaitInterval = 0,
		.enableWriteMask      = false
	};

	// Boot up MCU
	system_boot_up();

	// Test code
	FLEXSPI_SetFlashConfig(FLEXSPI, &deviceconfig, kFLEXSPI_PortA1);

	// Update LUT
	FLEXSPI_UpdateLUT(FLEXSPI, 0, NAND_FLEXSPI_LUT, 64);


	while(1)
	{
		IOPCTL_PinMuxSet(IOPCTL, BOARD_INITPINS_LEDR_PWM_PORT, BOARD_INITPINS_LEDR_PWM_PIN, LED_PWM_ON);
		IOPCTL_PinMuxSet(IOPCTL, BOARD_INITPINS_LEDR_PWM_PORT, BOARD_INITPINS_LEDG_PWM_PIN, LED_PWM_ON);
		IOPCTL_PinMuxSet(IOPCTL, BOARD_INITPINS_LEDR_PWM_PORT, BOARD_INITPINS_LEDB_PWM_PIN, LED_PWM_ON);
		SDK_DelayAtLeastUs(1000*500, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
		printf("\n\rhello world\n\r");
		IOPCTL_PinMuxSet(IOPCTL, BOARD_INITPINS_LEDR_PWM_PORT, BOARD_INITPINS_LEDR_PWM_PIN, LED_PWM_OFF);
		IOPCTL_PinMuxSet(IOPCTL, BOARD_INITPINS_LEDR_PWM_PORT, BOARD_INITPINS_LEDG_PWM_PIN, LED_PWM_OFF);
		IOPCTL_PinMuxSet(IOPCTL, BOARD_INITPINS_LEDR_PWM_PORT, BOARD_INITPINS_LEDB_PWM_PIN, LED_PWM_OFF);
		SDK_DelayAtLeastUs(1000*500, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
	}

	return 0;
}
