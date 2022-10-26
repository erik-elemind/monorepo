/*
 * Copyright 2020-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "flash_config.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flash_config"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
#if defined(BOOT_HEADER_ENABLE) && (BOOT_HEADER_ENABLE == 1)
#if defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".flash_conf"), used))
#elif defined(__ICCARM__)
#pragma location = ".flash_conf"
#endif

const flexspi_nor_config_t flexspi_config = {
    .memConfig =
        {
            .tag                 = FLASH_CONFIG_BLOCK_TAG,
            .version             = FLASH_CONFIG_BLOCK_VERSION,
            .csHoldTime          = 3,
            .csSetupTime         = 3,

            .deviceType    = 0x1,
            .sflashPadType = kSerialFlash_4Pads,
            .serialClkFreq = kFlexSpiSerialClk_SDR_48MHz,
            .sflashA1Size  = 0,
            .sflashA2Size  = 0,
            .sflashB1Size  = 0x1000000,
            .sflashB2Size  = 0,
            .lookupTable =
                {
                    //[0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x03, RADDR_SDR, FLEXSPI_1PAD, 0x18),
					//[1] = FLEXSPI_LUT_SEQ(READ_SDR, FLEXSPI_1PAD, 0x04, STOP_EXE, FLEXSPI_1PAD, 0),
					[0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xEB, RADDR_SDR, FLEXSPI_4PAD, 0x18),
					[1] = FLEXSPI_LUT_SEQ(MODE8_SDR, FLEXSPI_4PAD, 0xFF, DUMMY_SDR, FLEXSPI_4PAD, 0x04),
					[2] = FLEXSPI_LUT_SEQ(READ_SDR, FLEXSPI_4PAD, 0x04, STOP_EXE, FLEXSPI_1PAD, 0x00)

					/* Read Status*/
					//[4*1+0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x05, READ_SDR, FLEXSPI_1PAD, 0x04),

					/* Write Enable*/
					//[4*3+0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x06, STOP_EXE, FLEXSPI_1PAD, 0x00),
                },
        },
    .pageSize           = 0x100,
    .sectorSize         = 0x1000,
    .ipcmdSerialClkFreq = 1,
    .blockSize          = 0x10000,
};
#endif /* BOOT_HEADER_ENABLE */
