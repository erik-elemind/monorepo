/*
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Sept, 2021
 * Author:  Paul Adelsbach
 *
 * Description: BMA253 accelerometer interface layer.
 *
 * Platform and OS independent accerometer interface.
 * Handles basic power modes, fifo control, and motion detection.
 * Interface is designed to avoid exposing any internal register values.
 * See register specific header file for register definitions, and the 
 * datasheet for documentation.
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bma253-ds000.pdf
 */

#ifndef BMA253_H
#define BMA253_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

#define BMA253_I2C_ADDR   (0x18)

// Routines implemented by the caller which perform read and write operations
// using the system-specific APIs.
// These are blocking functions and return 0 for success and non-0 for failure.
typedef int (*bma253_reg_read_func_t)(uint8_t reg_addr, uint8_t* data, uint8_t len);
typedef int (*bma253_reg_write_func_t)(uint8_t reg_addr, uint8_t data);

typedef enum {
  BMA253_RANGE_2G,
  BMA253_RANGE_4G,
  BMA253_RANGE_8G,
  BMA253_RANGE_16G,
  BMA253_RANGE_NUM
} bma253_range_t;

typedef enum {
  BMA253_BW_7_81,   // 7.81hz, 64ms
  BMA253_BW_15_63,  // 15.63hz, 32ms
  BMA253_BW_31_25,  // 31.25hz, 16ms
  BMA253_BW_62_5,   // 62.5hz, 8ms
  BMA253_BW_125,    // 125hz, 4ms
  BMA253_BW_250,    // 250hz, 2ms
  BMA253_BW_500,    // 500hz, 1ms
  BMA253_BW_1000,    // 1000hz, 0.5ms
  BMA253_BW_NUM
} bma253_bandwidth_t;

typedef enum {
  BMA253_SLEEP_DUR_0_5, // 0.5ms
  BMA253_SLEEP_DUR_1,   // 1ms
  BMA253_SLEEP_DUR_2,   // 2ms, etc
  BMA253_SLEEP_DUR_4,
  BMA253_SLEEP_DUR_6,
  BMA253_SLEEP_DUR_10,
  BMA253_SLEEP_DUR_25,
  BMA253_SLEEP_DUR_50,
  BMA253_SLEEP_DUR_100,
  BMA253_SLEEP_DUR_500,
  BMA253_SLEEP_DUR_1000,
  BMA253_SLEEP_DUR_NUM
} bma253_sleep_dur_t;

typedef enum {
  BMA253_POWER_MODE_NORMAL,
  BMA253_POWER_MODE_LOW_POWER_1,
  BMA253_POWER_MODE_LOW_POWER_2,
  BMA253_POWER_MODE_SUSPEND,
  BMA253_POWER_MODE_DEEP_SUSPEND,
  BMA253_POWER_MODE_NUM
} bma253_power_mode_t;

// Fifo modes
typedef enum {
  BMA253_FIFO_MODE_FIFO,    // fill and stop
  BMA253_FIFO_MODE_STREAM,  // overwrite oldest without stopping
  BMA253_FIFO_MODE_BYPASS,  // bypass fifo
  BMA253_FIFO_MODE_NUM,
} bma253_fifo_mode_t;

// Return codes for this module
typedef enum {
  BMA253_STATUS_SUCCESS = 0,
  BMA253_STATUS_ERROR_INTERNAL,
  BMA253_STATUS_ERROR_NOT_INIT,
  BMA253_STATUS_ERROR_PARAMS,
} bma253_status_t;

// Accel sample struct
typedef union {
  struct {
    int16_t x;
    int16_t y;
    int16_t z;
  };
  uint8_t bytes[6];
} bma253_sample_t;
//static_assert(sizeof(bma253_sample_t) == 6, "x,y,z sample must be 6 bytes");

/** Initialize accelerometer driver.

  Initializes the software interface only. Does not access the chip.

  @param reg_read_func Function pointer to system specific read interface.
  @param reg_write_func Function pointer to system specific write interface.

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_init(bma253_reg_read_func_t reg_read_func, bma253_reg_write_func_t reg_write_func);


/** Read the chip ID.

  @param chipid Pointer to chipid, filled in by this function.

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_chipid_get(uint8_t* chipid);

/** Perform a softreset
  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_softreset(void);

/** Set the bandwidth setting of the chip

  @param bw Bandwidth value to set

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_bandwidth_set(bma253_bandwidth_t bw);

/** Set the power mode settings of the chip

  The BMA253 uses the sleep duration to set the effective sample rate.

  @param mode Power mode
  @param dur Sleep duration
  @param sleeptimer_en Flag to enable the sleeptimer. 
    Datasheet recommends setting this whenever using the fifo.

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_power_mode_set(bma253_power_mode_t mode, bma253_sleep_dur_t dur, bool sleeptimer_en);

/** Set the range setting of the chip

  @param range Range of acceleration to measure

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_range_set(bma253_range_t range);

/** Set the fifo setting of the chip

  @param mode Fifo mode to configure
  @param watermark Watermark value
  @param int_en Enable the watermark interrupt

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_fifo_config_set(bma253_fifo_mode_t mode, uint8_t watermark, bool int_en);

/** Get the samples from the fifo

  Reads samples until the fifo is empty or the provided buffer is full.

  @param samples Array in which to store samples
  @param num_samples Number of samples in the array on input. 
   Upon success, indicates the number of samples read. Set to 0 if none available.

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_fifo_samples_get(bma253_sample_t* samples, uint8_t* num_samples);

/** Get the temperature reading from the chip

  Output is a signed two's compliment number. 0x00 is 23 degrees C.

  @param temp Pointer to temperature, filled in by this function.

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_temp_get(int8_t* temp);

/** Set the slope settings in the chip

  This is also known as any-motion detection.

  @param thresh_mg Threshold in milli-g to check for. Set to 0 to use chip default.
  @param int_en Enable the slope interrupt

  @return BMA253_STATUS_SUCCESS if successful
 */
bma253_status_t bma253_slope_config_set(uint16_t thresh_mg, bool int_en);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif // BMA253_H

