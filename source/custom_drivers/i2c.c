/*
 * i2c.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger
 *
 * Description: I2C driver implementation.
*/
#include "i2c.h"

/// Internal wrapper for MCUXpresso I2C transfer
static status_t
i2c_mem_read_write(
  i2c_rtos_handle_t *handle,
  i2c_direction_t direction,
  uint8_t slave_address,
  uint32_t register_address,
  size_t register_address_size,
  uint8_t* p_data,
  size_t data_size
  )
{
  // Initialize the values for the read transfer:
  i2c_master_transfer_t i2c_master_transfer;
  memset(&i2c_master_transfer, 0, sizeof(i2c_master_transfer));

  i2c_master_transfer.direction      = direction;
  i2c_master_transfer.slaveAddress   = slave_address;
  i2c_master_transfer.subaddress     = register_address;
  i2c_master_transfer.subaddressSize = register_address_size;
  i2c_master_transfer.data           = p_data;
  i2c_master_transfer.dataSize       = data_size;
  i2c_master_transfer.flags          = kI2C_TransferDefaultFlag;

  // Execute transfer
  status_t status = I2C_RTOS_Transfer(handle, &i2c_master_transfer);

  return status;
}

status_t
i2c_mem_read(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint32_t register_address,
  size_t register_address_size,
  uint8_t* p_data,
  size_t data_size
  )
{
  return i2c_mem_read_write(handle, kI2C_Read, slave_address,
    register_address, register_address_size, p_data, data_size);
}

status_t
i2c_mem_write(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint32_t register_address,
  size_t register_address_size,
  uint8_t* p_data,
  size_t data_size
  )
{
  return i2c_mem_read_write(handle, kI2C_Write, slave_address,
    register_address, register_address_size, p_data, data_size);
}

/* Send out just the address, and wait for an ACK.

   Uses the lower-level I2C API. Breaks open the I2C handle to get the
   mutex for the bus, since the normal API doesn't allow us to do a
   non-data-transfer I2C transaction.
 */
status_t
i2c_is_device_ready(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address
  )
{
  // Lock resource mutex
  if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE) {
    return kStatus_I2C_Busy;
  }

  // Send address
  I2C_MasterStart(handle->base, slave_address, kI2C_Write);

  // Wait until address sent out.
  uint8_t i2c_status;
  do {
    i2c_status = I2C_GetStatusFlags(handle->base);
  } while ((i2c_status & I2C_STAT_MSTPENDING_MASK) == 0);

  // Check state--anything other than a NAK is a success
  status_t status = kStatus_Success;
  uint8_t i2c_master_state =
    (i2c_status & I2C_STAT_MSTSTATE_MASK) >> I2C_STAT_MSTSTATE_SHIFT;
  if (i2c_master_state == I2C_STAT_MSTCODE_NACKADR) {
    status = kStatus_I2C_Nak;
  }
  else {
    status = kStatus_Success;
  }

  /* End transaction. We need to do this even if the slave NAKed, in
     order to get the LPC I2C state machine back to the right
     state for future reads/writes. */
  I2C_MasterStop(handle->base);

  // Unlock resource mutex
  xSemaphoreGive(handle->mutex);

  return status;
}

status_t
i2c_mem_read_byte(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint8_t register_address,
  uint8_t* p_data
  )
{
  return i2c_mem_read(handle, slave_address, register_address, 1, p_data, 1);
}

status_t
i2c_mem_write_byte(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint8_t register_address,
  uint8_t data
  )
{
  return i2c_mem_write(handle, slave_address, register_address, 1, &data, 1);
}
