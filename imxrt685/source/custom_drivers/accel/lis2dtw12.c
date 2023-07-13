#include "lis2dtw12.h"
#include "lis2dtw12_reg.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

// Global context
static stmdev_ctx_t g_context;

int32_t lis2dtw12_init(stmdev_read_ptr reg_read_func, stmdev_write_ptr reg_write_func)
{
  g_context.read_reg  = reg_read_func;
  g_context.write_reg = reg_write_func;
  return 0;
}

int32_t lis2dtw12_chipid_get(uint8_t* chipid)
{
  if (NULL == g_context.read_reg) {
    return -1;
  }

  return lis2dtw12_device_id_get(&g_context, chipid);
}

int32_t lis2dtw12_softreset(void)
{
  if (NULL == g_context.write_reg) {
    return -1;
  }

  return lis2dtw12_reset_set(&g_context, 1);
}

int32_t lis2dtw12_power_set(lis2dtw12_mode_t mode, lis2dtw12_odr_t rate)
{
  if (NULL == g_context.write_reg) {
    return -1;
  }

  if (lis2dtw12_power_mode_set(&g_context, mode)) {
    return -1;
  }
  
  return lis2dtw12_data_rate_set(&g_context, rate);
}

int32_t lis2dtw12_range_set(lis2dtw12_fs_t range)
{
  if (NULL == g_context.write_reg) {
    return -1;
  }

  return lis2dtw12_full_scale_set(&g_context, range);
}

int32_t lis2dtw12_fifo_config_set(lis2dtw12_fmode_t mode, uint8_t watermark, bool int_en)
{
  if (NULL == g_context.write_reg) {
    return -1;
  }

  if (lis2dtw12_data_rate_set(&g_context, LIS2DTW12_XL_ODR_25Hz)) {
    return -1;
  }
  
  if (lis2dtw12_power_mode_set(&g_context, LIS2DTW12_CONT_LOW_PWR_12bit)) {
    return -1;
  }
  
  if (lis2dtw12_fifo_mode_set(&g_context, mode)) {
    return -1;
  }

  if (lis2dtw12_fifo_watermark_set(&g_context, watermark)) {
    return -1;
  }

  if (int_en) {
    lis2dtw12_ctrl4_int1_pad_ctrl_t ctrl;
    ctrl.int1_fth = 1;
    if (lis2dtw12_pin_int1_route_set(&g_context, &ctrl)) {
      return -1;
    }
  }
  
  return 0;
}

int32_t lis2dtw12_fifo_samples_get(lis2dtw12_sample_t* samples, uint8_t* num_samples)
{
  if (NULL == g_context.read_reg) {
    return -1;
  }

  uint8_t fifo_level;
  uint8_t ovr_flag;

  if (lis2dtw12_fifo_data_level_get(&g_context, &fifo_level)) {
    return -1;
  }
  
  if (lis2dtw12_fifo_ovr_flag_get(&g_context, &ovr_flag)) {
    return -1;
  }

  // handle overrun, if any
  if (ovr_flag) {
    return -1;
  }

  // check for valid length in the buffer
  if (fifo_level > *num_samples) {
    return -1;
  }

  // read all entries from the fifo
  uint8_t samples_to_read = MIN(*num_samples, fifo_level);
  for (uint8_t i=0; i<samples_to_read; i++) {
    // read full entry from REG_ADDR_FIFO_DATA
    if (lis2dtw12_acceleration_raw_get(&g_context, (int16_t*)&samples[i].bytes)) {
      return -1;
    }
    *num_samples = i+1;
  }

  return 0;
}

int32_t lis2dtw12_temp_get(int16_t* temp)
{
  if (NULL == g_context.read_reg) {
    return -1;
  }

  return lis2dtw12_temperature_raw_get(&g_context, temp);
}

int32_t lis2dtw12_slope_config_set(uint16_t thresh, bool int_en)
{
  if (NULL == g_context.write_reg || NULL == g_context.read_reg) {
    return -1;
  }

  if (lis2dtw12_data_rate_set(&g_context, LIS2DTW12_XL_ODR_25Hz)) {
    return -1;
  }

  if (lis2dtw12_wkup_threshold_set(&g_context, thresh)) {
    return -1;
  }

  if (int_en) {
    lis2dtw12_ctrl4_int1_pad_ctrl_t ctrl;
    ctrl.int1_wu = 1;
    if (lis2dtw12_pin_int1_route_set(&g_context, &ctrl)) {
      return -1;
    }
  }

  return 0;
}

