/*
 * i2c.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger
 *
 * Description:    I2C driver.
 *
 * Wraps MCUXpresso I2C FreeRTOS driver to make it easier to
 * use. Modeled after STM32 HAL I2C API.
*/
#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "fsl_i2c_freertos.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Lowest valid 7-bit I2C address
#define I2C_ADDRESS_MIN 0x07

/// Highest valid 7-bit I2C address
#define I2C_ADDRESS_MAX 0x77

/** I2C read.

    @param handle I2C handle from I2C_RTOS_Init()
    @param slave_address 7-bit I2C address, right-aligned
    @param register_address Register address on device
    @param register_address_size Number of bytes in register address (max: 4)
    @param p_data Pointer to buffer to read into
    @param data_size Number of bytes to read

    @return kStatus_Success if successful
 */
status_t
i2c_mem_read(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint32_t register_address,
  size_t register_address_size,
  uint8_t* p_data,
  size_t data_size
  );

/** I2C write.

    @param handle I2C handle from I2C_RTOS_Init()
    @param slave_address 7-bit I2C address, right-aligned
    @param register_address Register address on device
    @param register_address_size Number of bytes in register address (max: 4)
    @param p_data Pointer to buffer to write
    @param data_size Number of bytes to write

    @return kStatus_Success if successful
 */
status_t
i2c_mem_write(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint32_t register_address,
  size_t register_address_size,
  uint8_t* p_data,
  size_t data_size
  );

/** I2C device ready check.

    Send device address, and check for an ACK.

    @param handle I2C handle from I2C_RTOS_Init()
    @param slave_address 7-bit I2C address, right-aligned

    @return kStatus_Success if device is ready
 */
status_t
i2c_is_device_ready(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address
  );

/** I2C read one byte.

    Convenience function for single-byte read from a register with a
    one-byte address.

    @param handle I2C handle from I2C_RTOS_Init()
    @param slave_address 7-bit I2C address, right-aligned
    @param register_address Register address on device (one byte)
    @param p_data Pointer to buffer to read one byte into

    @return kStatus_Success if successful
 */
status_t
i2c_mem_read_byte(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint8_t register_address,
  uint8_t* p_data
  );

/** I2C write one byte.

    Convenience function for single-byte write to a register with a
    one-byte address.

    @param handle I2C handle from I2C_RTOS_Init()
    @param slave_address 7-bit I2C address, right-aligned
    @param register_address Register address on device (one byte)
    @param data Byte to write

    @return kStatus_Success if successful
 */
status_t
i2c_mem_write_byte(
  i2c_rtos_handle_t *handle,
  uint8_t slave_address,
  uint8_t register_address,
  uint8_t data
  );

/// @todo BH Add convenience functions for 2- and 4-byte read/write

#ifdef __cplusplus
}
#endif

#endif  // I2C_H
