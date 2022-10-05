/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Platform-specific routines for the generic SPI NAND driver.
*/
#ifndef NAND_PLATFORM_H
#define NAND_PLATFORM_H

#include "nand_W25N04KW.h"

#ifdef __cplusplus
extern "C" {
#endif

// Delay for delay_ms (delay_ms may be zero for a simple thread yield).
void nand_platform_yield_delay(int delay_ms);

// Return 0 if command and response completed succesfully, or <0 error code
int nand_platform_command_response(
  uint8_t* p_command,
  uint8_t command_len,
  uint8_t* p_data,
  uint16_t data_len
  );

// Return 0 if command and data completed succesfully, or <0 error code
int nand_platform_command_with_data(
  uint8_t* p_command,
  uint8_t command_len,
  uint8_t* p_data,
  uint16_t data_len
  );

// Callbacks for the DMA ISRs
void nand_platform_spi_dma_rx_complete(void);

// Init
int nand_platform_init(void);

#ifdef __cplusplus
}
#endif

#endif  // NAND_PLATFORM_H
