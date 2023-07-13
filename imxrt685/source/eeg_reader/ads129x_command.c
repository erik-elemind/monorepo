/**
 * send and receive commands from TI ADS129x chips.
 *
 * Copyright (c) 2013 by Adam Feuer <adam@adamfeuer.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * Original Name: adsCommand.cpp
 * New Name:      ads129x_command.c
 *
 * Downloaded from: https://github.com/starcat-io/hackeeg-driver-arduino/
 * Downloaded on: February, 2020
 *
 * Modified:     Jul, 2020
 * Modified by:  Gansheng Ou, David Wang
 *
 * Description: SPI command for ADS129x
 */

#include "fsl_spi.h"
#include "fsl_spi_freertos.h"
#include "config.h"
#include "loglevels.h"
#include "ads129x_command.h"
#include "ads129x_regs.h"

static const char *TAG = "ads";  // Logging prefix for this module

ads_status ads_send_command(uint8_t cmd)
{
  /* Setup TX/RX buffers */
  uint8_t tx_cmd[1] = {0};
  tx_cmd[0] = cmd;
  uint8_t rx_cmd[1] = {0}; // expect no meaningful rx bytes

  /* Setup master transfer */
  status_t status;
  spi_transfer_t masterXfer = {0};
  masterXfer.txData   = tx_cmd;
  masterXfer.dataSize = 1;
  masterXfer.rxData   = rx_cmd;
  masterXfer.configFlags |= kSPI_FrameAssert;

#if 0 // TODO: Remove
  /* Start master transfer */
  status = SPI_RTOS_Transfer(&EEG_SPI_RTOS_HANDLE, &masterXfer);
  if (status != kStatus_Success)
  {
    LOGD(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }
#endif
  status = SPI_MasterTransferBlocking(SPI_EEG_BASE, &masterXfer);
  if (status != kStatus_Success)
  {
    LOGD(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }
  return ADS_STATUS_SUCCESS;
}

#if 0
// TODO: Fix this method... How to leave CS active?
ads_status ads_send_command_leave_cs_active(uint8_t cmd)
{
  /* Setup TX/RX buffers */
  uint8_t tx_cmd[1] = {0};
  tx_cmd[0] = cmd;
  uint8_t rx_cmd[1] = {0}; // expect no meaningful rx bytes

  /* Setup master transfer */
  status_t status;
  spi_transfer_t masterXfer = {0};
  masterXfer.txData   = tx_cmd;
  masterXfer.dataSize = 1;
  masterXfer.rxData   = rx_cmd;
  masterXfer.configFlags |= kSPI_FrameAssert;

  /* Start master transfer */
  status = SPI_RTOS_Transfer(&EEG_SPI_RTOS_HANDLE, &masterXfer);
  if (status != kStatus_Success)
  {
    LOGD(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }
  return ADS_STATUS_SUCCESS;
}
#endif

ads_status ads_wreg(uint8_t reg, uint8_t val)
{
  /* Setup TX/RX buffers */
  uint8_t tx_cmd[3] = {0};
  tx_cmd[0] = WREG | reg; // register address to write
  tx_cmd[1] = 0;          // (number of registers to write) - 1
  tx_cmd[2] = val;        // value to write
  uint8_t rx_cmd[3] = {0}; // expect no meaningful rx bytes

  /* Setup master transfer */
  status_t status;
  spi_transfer_t masterXfer = {0};
  masterXfer.txData   = tx_cmd;
  masterXfer.dataSize = 3;
  masterXfer.rxData   = rx_cmd;
  masterXfer.configFlags |= kSPI_FrameAssert;

#if 0 // TODO: Remove
  /* Start master transfer */
  status = SPI_RTOS_Transfer(&EEG_SPI_RTOS_HANDLE, &masterXfer);
  if (status != kStatus_Success)
  {
    LOGV(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }
#endif
  status = SPI_MasterTransferBlocking(SPI_EEG_BASE, &masterXfer);
  if (status != kStatus_Success)
  {
    LOGV(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }

  return ADS_STATUS_SUCCESS;
}

ads_status ads_rreg(uint8_t reg, uint8_t *val)
{
  /* Setup TX/RX buffers */
  uint8_t tx_cmd[3] = {0}; // 3 bytes
  tx_cmd[0] = RREG | reg; // register address to read
  tx_cmd[1] = 0;          // (number of registers to read) - 1
  tx_cmd[2] = 0;          // transmit 1 byte to receive 1 byte.
  uint8_t rx_cmd[3] = {0}; // expect only 1 meaningful rx byte

  /* Setup master transfer */
  status_t status;
  spi_transfer_t masterXfer = {0};
  masterXfer.txData   = tx_cmd;
  masterXfer.dataSize = 3;
  masterXfer.rxData   = rx_cmd;
  masterXfer.configFlags |= kSPI_FrameAssert;

  /* Start master transfer */
#if 0 // TODO: Remove
  status = SPI_RTOS_Transfer(&EEG_SPI_RTOS_HANDLE, &masterXfer);
  if (status != kStatus_Success)
  {
    LOGD(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }
#endif
  status = SPI_MasterTransferBlocking(SPI_EEG_BASE, &masterXfer);
  if (status != kStatus_Success)
  {
    LOGV(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }

  *val = rx_cmd[2];
  return ADS_STATUS_SUCCESS;
}
