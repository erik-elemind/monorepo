#ifndef LIS2DTW12_H
#define LIS2DTW12_H

#include <stdbool.h>

#include "lis2dtw12_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

// Accel sample struct
typedef union {
  struct {
    int16_t x;
    int16_t y;
    int16_t z;
  };
  uint8_t bytes[6];
} lis2dtw12_sample_t;

/** Initialize accelerometer driver.

  Initializes the software interface only. Does not access the chip.

  @param reg_read_func Function pointer to system specific read interface.
  @param reg_write_func Function pointer to system specific write interface.

  @return 0 if successful
 */
int32_t lis2dtw12_init(stmdev_read_ptr reg_read_func, stmdev_write_ptr reg_write_func);
  
/** Read the chip ID.

  @param chipid Pointer to device_id, filled in by this function.

  @return 0 if successful
 */
int32_t lis2dtw12_chipid_get(uint8_t* chipid);

/** Perform a softreset
  @return 0 if successful
 */
int32_t lis2dtw12_softreset(void);

/** Set the power mode settings of the chip

  The LIS2DTW12 uses the sleep duration to set the effective sample rate.

  @param mode Power mode
  @param rate Data rate

  @return 0 if successful
 */
  int32_t lis2dtw12_power_set(lis2dtw12_mode_t mode, lis2dtw12_odr_t rate);

/** Set the range setting of the chip

  @param range Range of acceleration to measure

  @return 0 if successful
 */
int32_t lis2dtw12_range_set(lis2dtw12_fs_t range);

/** Set the fifo setting of the chip

  @param mode Fifo mode to configure
  @param watermark Watermark value
  @param int_en Enable the watermark interrupt

  @return 0 if successful
 */
int32_t lis2dtw12_fifo_config_set(lis2dtw12_fmode_t mode, uint8_t watermark, bool int_en);

/** Get the samples from the fifo

  Reads samples until the fifo is empty or the provided buffer is full.

  @param samples Array in which to store samples
  @param num_samples Number of samples in the array on input. 
   Upon success, indicates the number of samples read. Set to 0 if none available.

  @return 0 if successful
 */
int32_t lis2dtw12_fifo_samples_get(lis2dtw12_sample_t* samples, uint8_t* num_samples);

/** Get the temperature reading from the chip

  Output is a signed two's compliment number. 0x00 is 23 degrees C.

  @param temp Pointer to temperature, filled in by this function.

  @return 0 if successful
 */
int32_t lis2dtw12_temp_get(int16_t* temp);

/** Set the slope settings in the chip

  This is also known as any-motion detection.

  @param thresh_mg Threshold to check for.  1 LSB = 1/64 of FS
  @param int_en Enable the slope interrupt

  @return 0 if successful
 */
int32_t lis2dtw12_slope_config_set(uint16_t thresh, bool int_en);

#ifdef __cplusplus
}
#endif

#endif // LIS2DTW12_H
