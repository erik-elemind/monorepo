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

#include "bma253.h"
#include "bma253_regs.h"

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
typedef struct {
  bma253_reg_read_func_t  reg_read_func;
  bma253_reg_write_func_t reg_write_func;
} bma253_context_t;
static bma253_context_t g_context;

bma253_status_t bma253_init(bma253_reg_read_func_t reg_read_func, bma253_reg_write_func_t reg_write_func)
{
  g_context.reg_read_func  = reg_read_func;
  g_context.reg_write_func = reg_write_func;
  return BMA253_STATUS_SUCCESS;
}

bma253_status_t bma253_chipid_get(uint8_t* chipid)
{
  if (NULL == g_context.reg_read_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  if (0 == g_context.reg_read_func(REG_ADDR_BGW_CHIPID, chipid, sizeof(*chipid))) {
    return BMA253_STATUS_SUCCESS;
  }

  return BMA253_STATUS_ERROR_INTERNAL;
}

bma253_status_t bma253_softreset(void)
{
  if (NULL == g_context.reg_write_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  if (0 == g_context.reg_write_func(REG_ADDR_BGW_SOFTRESET, REG_DATA_SOFTRESET)) {
    return BMA253_STATUS_SUCCESS;
  }

  return BMA253_STATUS_ERROR_INTERNAL;
}

bma253_status_t bma253_bandwidth_set(bma253_bandwidth_t bw)
{
  if (NULL == g_context.reg_write_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  if (bw >= BMA253_BW_NUM) {
    return BMA253_STATUS_ERROR_PARAMS;
  }

  // map public types to register values
  static const uint8_t bw_map[] = {
    REG_DATA_BW_7_81,     // BMA253_BW_7_81
    REG_DATA_BW_15_63,    // BMA253_BW_15_63
    REG_DATA_BW_31_25,    // BMA253_BW_31_25
    REG_DATA_BW_62_5,     // BMA253_BW_62_5
    REG_DATA_BW_125,      // BMA253_BW_125
    REG_DATA_BW_250,      // BMA253_BW_250
    REG_DATA_BW_500,      // BMA253_BW_500
    REG_DATA_BW_1000,     // BMA253_BW_1000
  };
  static_assert(ARRAY_SIZE(bw_map) == BMA253_BW_NUM, "bw_map size");

  reg_pmu_bw_t reg = { .bw = bw_map[bw] };
  int result = g_context.reg_write_func(REG_ADDR_PMU_BW, reg.all);
  return 0 == result ? BMA253_STATUS_SUCCESS : BMA253_STATUS_ERROR_INTERNAL;
}

bma253_status_t bma253_power_mode_set(bma253_power_mode_t mode, bma253_sleep_dur_t dur, bool sleeptimer_en)
{
  if (NULL == g_context.reg_write_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  if (mode >= BMA253_POWER_MODE_NUM || dur >= BMA253_SLEEP_DUR_NUM) {
    return BMA253_STATUS_ERROR_PARAMS;
  }

  reg_pmu_lpw_t pmu_lpw = {0};
  reg_pmu_low_power_t pmu_low_power = {0};

  // use timer or event driven.
  pmu_low_power.sleeptimer_mode = sleeptimer_en;

  switch (mode) {
    case BMA253_POWER_MODE_NORMAL:
      // nothing to do
      break;
    case BMA253_POWER_MODE_LOW_POWER_1:
      pmu_lpw.lowpower_en = 1;
      break;
    case BMA253_POWER_MODE_LOW_POWER_2:
      pmu_lpw.lowpower_en = 1;
      // lowpower mode 2 enables the fifo to be readable at all times.
      pmu_low_power.lowpower_mode = 1;
      break;
    case BMA253_POWER_MODE_SUSPEND:
      pmu_lpw.suspend = 1;
      break;
    case BMA253_POWER_MODE_DEEP_SUSPEND:
      pmu_lpw.deep_suspend  = 1;
      break;
    default:
      return BMA253_STATUS_ERROR_PARAMS;
  }

  // map public types to register values
  static const uint8_t sleep_dur_map[] = {
    REG_DATA_SLEEP_DUR_0_5,   // BMA253_SLEEP_DUR_0_5
    REG_DATA_SLEEP_DUR_1,     // BMA253_SLEEP_DUR_1
    REG_DATA_SLEEP_DUR_2,     // BMA253_SLEEP_DUR_2
    REG_DATA_SLEEP_DUR_4,     // BMA253_SLEEP_DUR_4
    REG_DATA_SLEEP_DUR_6,     // BMA253_SLEEP_DUR_6
    REG_DATA_SLEEP_DUR_10,    // BMA253_SLEEP_DUR_10
    REG_DATA_SLEEP_DUR_25,    // BMA253_SLEEP_DUR_25
    REG_DATA_SLEEP_DUR_50,    // BMA253_SLEEP_DUR_50
    REG_DATA_SLEEP_DUR_100,   // BMA253_SLEEP_DUR_100
    REG_DATA_SLEEP_DUR_500,   // BMA253_SLEEP_DUR_500
    REG_DATA_SLEEP_DUR_1000,  // BMA253_SLEEP_DUR_1000
  };
  static_assert(ARRAY_SIZE(sleep_dur_map) == BMA253_SLEEP_DUR_NUM, "sleep_dur_map size");

  // lookup the register value
  pmu_lpw.sleep_dur = sleep_dur_map[dur];

  if (0 == g_context.reg_write_func(REG_ADDR_PMU_LPW, pmu_lpw.all) && 
      0 == g_context.reg_write_func(REG_ADDR_PMU_LOW_POWER, pmu_low_power.all)) {
    return BMA253_STATUS_SUCCESS;
  }

  return BMA253_STATUS_ERROR_INTERNAL;
}

bma253_status_t bma253_range_set(bma253_range_t range)
{
  if (NULL == g_context.reg_write_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  if (range >= BMA253_RANGE_NUM) {
    return BMA253_STATUS_ERROR_PARAMS;
  }

  static const uint8_t range_map[] = {
    REG_DATA_PMU_RANGE_2G,  // BMA253_RANGE_2G
    REG_DATA_PMU_RANGE_4G,  // BMA253_RANGE_4G
    REG_DATA_PMU_RANGE_8G,  // BMA253_RANGE_8G
    REG_DATA_PMU_RANGE_16G, // BMA253_RANGE_16G
  };
  static_assert(ARRAY_SIZE(range_map) == BMA253_RANGE_NUM, "range_map size");

  reg_pmu_range_t reg = { .range = range_map[range] };
  if (0 == g_context.reg_write_func(REG_ADDR_PMU_RANGE, reg.all)) {
    return BMA253_STATUS_SUCCESS;
  }

  return BMA253_STATUS_ERROR_INTERNAL;
}

bma253_status_t bma253_fifo_config_set(bma253_fifo_mode_t mode, uint8_t watermark, bool int_en)
{
  if (NULL == g_context.reg_write_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  if (mode >= BMA253_FIFO_MODE_NUM || watermark >= REG_DATA_FIFO_DEPTH) {
    return BMA253_STATUS_ERROR_PARAMS;
  }

  reg_fifo_config_0_t fifo_config_0 = {
    .fifo_water_mark_level_trigger_retain = watermark
  };

  static const uint8_t fifo_mode_map[] = {
    REG_DATA_FIFO_MODE_FIFO,    // BMA253_FIFO_MODE_FIFO
    REG_DATA_FIFO_MODE_STREAM,  // BMA253_FIFO_MODE_STREAM
    REG_DATA_FIFO_MODE_BYPASS,  // BMA253_FIFO_MODE_BYPASS
  };
  static_assert(ARRAY_SIZE(fifo_mode_map) == BMA253_FIFO_MODE_NUM, "fifo_mode_map size");

  reg_fifo_config_1_t fifo_config_1 = {
    .fifo_data_select = 0,  // always save all 3 axes
    .fifo_mode = fifo_mode_map[mode],
  };

  // enable or disable the interrupt
  reg_int_en_1_t int_en_1 = { .int_fwm_en = int_en };
  reg_int_map_1_t int_map_1 = { .int1_fwm = int_en };

  if (0 == g_context.reg_write_func(REG_ADDR_FIFO_CONFIG_0, fifo_config_0.all) && 
      0 == g_context.reg_write_func(REG_ADDR_FIFO_CONFIG_1, fifo_config_1.all) &&
      0 == g_context.reg_write_func(REG_ADDR_INT_EN_1, int_en_1.all) &&
      0 == g_context.reg_write_func(REG_ADDR_INT_MAP_1, int_map_1.all)) {
    return BMA253_STATUS_SUCCESS;
  }

  return BMA253_STATUS_ERROR_INTERNAL;
}

bma253_status_t bma253_fifo_samples_get(bma253_sample_t* samples, uint8_t* num_samples)
{
  if (NULL == g_context.reg_read_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  reg_fifo_status_t fifo_status = {0};

  if (0 != g_context.reg_read_func(REG_ADDR_FIFO_STATUS, &fifo_status.all, sizeof(fifo_status))) {
    return BMA253_STATUS_ERROR_INTERNAL;
  }

  // handle overrun, if any
  if (fifo_status.fifo_overrun) {
    return BMA253_STATUS_ERROR_INTERNAL;
  }

  // check for valid length in the buffer
  if (fifo_status.fifo_frame_counter > *num_samples) {
    return BMA253_STATUS_ERROR_PARAMS;
  }

  // read all entries from the fifo
  uint8_t samples_to_read = MIN(*num_samples, fifo_status.fifo_frame_counter);
  for (uint8_t i=0; i<samples_to_read; i++) {
    // read full entry from REG_ADDR_FIFO_DATA
    if (0 != g_context.reg_read_func(REG_ADDR_FIFO_DATA, (uint8_t*)&samples[i].bytes, sizeof(samples[i]))) {
      return BMA253_STATUS_ERROR_INTERNAL;
    }
    *num_samples = i+1;
  }

  return BMA253_STATUS_SUCCESS;
}

bma253_status_t bma253_temp_get(int8_t* temp)
{
  if (NULL == g_context.reg_read_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  if (0 == g_context.reg_read_func(REG_ADDR_ACCD_TEMP, (uint8_t*)temp, sizeof(*temp))) {
    return BMA253_STATUS_SUCCESS;
  }

  return BMA253_STATUS_ERROR_INTERNAL;
}

bma253_status_t bma253_slope_config_set(uint16_t thresh_mg, bool int_en)
{
  if (NULL == g_context.reg_write_func || NULL == g_context.reg_read_func) {
    return BMA253_STATUS_ERROR_NOT_INIT;
  }

  reg_int_en_0_t int_en_0 = { 
    .slope_en_x = int_en,
    .slope_en_y = int_en,
    .slope_en_z = int_en,
  };
  reg_int_map_0_t int_map_0 = {
    .int1_slope = int_en,
  };

  reg_int_5_t int_5 = {
    .slope_dur = 2,
  };

  if (0 == thresh_mg) {
    // Use the default threshold in the chip
  } 
  else {
    // Read the range setting to determine the threshold scaling factor
    reg_pmu_range_t pmu_range = {0};
    if (0 != g_context.reg_read_func(REG_ADDR_PMU_RANGE, &pmu_range.all, sizeof(pmu_range.all))) {
      return BMA253_STATUS_ERROR_INTERNAL;
    }

    // Scale the threshold based on range
    reg_int_6_t int_6 = {0};
    switch (pmu_range.range) {
      case BMA253_RANGE_2G:
        // approx divide by 3.91
        int_6.slope_th = thresh_mg >> 2;
        break;
      case BMA253_RANGE_4G:
        // approx divide by 7.81
        int_6.slope_th = thresh_mg >> 3;
        break;
      case BMA253_RANGE_8G:
        // approx divide by 15.63
        int_6.slope_th = thresh_mg >> 4;
        break;
      case BMA253_RANGE_16G:
        // approx divide by 31.25
        int_6.slope_th = thresh_mg >> 5;
        break;
      default:
        // unknown, use default
        int_6.slope_th = 0x14;
        break;
    }

    if (0 == g_context.reg_write_func(REG_ADDR_INT_6, int_6.all)) {
      return BMA253_STATUS_SUCCESS;
    }
  }

  if (0 == g_context.reg_write_func(REG_ADDR_INT_5, int_5.all) &&
      0 == g_context.reg_write_func(REG_ADDR_INT_EN_0, int_en_0.all) && 
      0 == g_context.reg_write_func(REG_ADDR_INT_MAP_0, int_map_0.all)) {
    return BMA253_STATUS_SUCCESS;
  }

  return BMA253_STATUS_ERROR_INTERNAL;
}
