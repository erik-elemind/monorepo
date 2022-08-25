/*
 * BMA253 register definitions.
 *
 * Created: Sept, 2021
 * Author:  Paul Adelsbach
 *
 * Intended to be included by bma253.c only. External callers should not 
 * need to include this file.
 * This file uses names from the datasheet wherever possible for easy searching.
 * But note that not all registers are defined and implemented.
 */

#ifndef BMA253_REGS_H
#define BMA253_REGS_H

#include <stdint.h>
#include <assert.h>

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

//
// Register addresses
//
#define REG_ADDR_BGW_CHIPID     (0x00)
#define REG_ADDR_ACCD_X_LSB     (0x02)
#define REG_ADDR_ACCD_X_MSB     (0x03)
#define REG_ADDR_ACCD_Y_LSB     (0x04)
#define REG_ADDR_ACCD_Y_MSB     (0x05)
#define REG_ADDR_ACCD_Z_LSB     (0x06)
#define REG_ADDR_ACCD_Z_MSB     (0x07)
#define REG_ADDR_ACCD_TEMP      (0x08)
#define REG_ADDR_INT_STATUS_0   (0x09)
#define REG_ADDR_INT_STATUS_1   (0x0A)
#define REG_ADDR_INT_STATUS_2   (0x0B)
#define REG_ADDR_INT_STATUS_3   (0x0C)
#define REG_ADDR_FIFO_STATUS    (0x0E)
#define REG_ADDR_PMU_RANGE      (0x0F)
#define REG_ADDR_PMU_BW         (0x10)
#define REG_ADDR_PMU_LPW        (0x11)
#define REG_ADDR_PMU_LOW_POWER  (0x12)
#define REG_ADDR_ACCD_HBW       (0x13)
#define REG_ADDR_BGW_SOFTRESET  (0x14)
#define REG_ADDR_INT_EN_0       (0x16)
#define REG_ADDR_INT_EN_1       (0x17)
#define REG_ADDR_INT_EN_2       (0x18)
#define REG_ADDR_INT_MAP_0      (0x19)
#define REG_ADDR_INT_MAP_1      (0x1A)
#define REG_ADDR_INT_MAP_2      (0x1B)
#define REG_ADDR_INT_SRC        (0x1E)
#define REG_ADDR_INT_OUT_CTRL   (0x20)
#define REG_ADDR_INT_RST_LATCH  (0x21)
#define REG_ADDR_INT_0          (0x22)
#define REG_ADDR_INT_1          (0x23)
#define REG_ADDR_INT_2          (0x24)
#define REG_ADDR_INT_3          (0x25)
#define REG_ADDR_INT_4          (0x26)
#define REG_ADDR_INT_5          (0x27)
#define REG_ADDR_INT_6          (0x28)
#define REG_ADDR_INT_7          (0x29)
#define REG_ADDR_INT_8          (0x2A)
#define REG_ADDR_INT_9          (0x2B)
#define REG_ADDR_INT_A          (0x2C)
#define REG_ADDR_INT_B          (0x2D)
#define REG_ADDR_INT_C          (0x2E)
#define REG_ADDR_INT_D          (0x2F)
#define REG_ADDR_FIFO_CONFIG_0  (0x30)
#define REG_ADDR_PMU_SELF_TEST  (0x32)
#define REG_ADDR_TRIM_NVM_CTRL  (0x33)
#define REG_ADDR_BGW_SPI3_WDT   (0x34)
#define REG_ADDR_OFC_CTRL       (0x36)
#define REG_ADDR_OFC_SETTING    (0x37)
#define REG_ADDR_OFC_OFFSET_X   (0x38)
#define REG_ADDR_OFC_OFFSET_Y   (0x39)
#define REG_ADDR_OFC_OFFSET_Z   (0x3A)
#define REG_ADDR_TRIM_GP1       (0x3C)
#define REG_ADDR_FIFO_CONFIG_1  (0x3E)
#define REG_ADDR_FIFO_DATA      (0x3F)

//
// Register fields
//

// This is the expected value in REG_ADDR_BGW_CHIPID
#define REG_DATA_BGW_CHIPID     (0xFA)

// Write this value to REG_ADDR_BGW_SOFTRESET to reset the chip
#define REG_DATA_SOFTRESET      (0xB6)

// REG_ADDR_PMU_BW
typedef union {
  struct {
    uint8_t bw  : 5;
    uint8_t     : 3;
  };
  uint8_t all;
} reg_pmu_bw_t;
//static_assert(sizeof(reg_pmu_bw_t) == 1, "Register must be 1 byte");

#define REG_DATA_BW_7_81    (0b01000)
#define REG_DATA_BW_15_63   (0b01001)
#define REG_DATA_BW_31_25   (0b01010)
#define REG_DATA_BW_62_5    (0b01011)
#define REG_DATA_BW_125     (0b01100)
#define REG_DATA_BW_250     (0b01101)
#define REG_DATA_BW_500     (0b01110)
#define REG_DATA_BW_1000    (0b01111)

// REG_ADDR_PMU_LPW
typedef union {
  struct {
    uint8_t               : 1;
    uint8_t sleep_dur     : 4;
    uint8_t deep_suspend  : 1;
    uint8_t lowpower_en   : 1;
    uint8_t suspend       : 1;
  };
  uint8_t all;
} reg_pmu_lpw_t;
//static_assert(sizeof(reg_pmu_lpw_t) == 1,
//  "Register must be 1 byte");

#define REG_DATA_SLEEP_DUR_0_5    (0b0000)
#define REG_DATA_SLEEP_DUR_1      (0b0110)
#define REG_DATA_SLEEP_DUR_2      (0b0111)
#define REG_DATA_SLEEP_DUR_4      (0b1000)
#define REG_DATA_SLEEP_DUR_6      (0b1001)
#define REG_DATA_SLEEP_DUR_10     (0b1010)
#define REG_DATA_SLEEP_DUR_25     (0b1011)
#define REG_DATA_SLEEP_DUR_50     (0b1100)
#define REG_DATA_SLEEP_DUR_100    (0b1101)
#define REG_DATA_SLEEP_DUR_500    (0b1110)
#define REG_DATA_SLEEP_DUR_1000   (0b1111)

// REG_ADDR_PMU_LOW_POWER
typedef union {
  struct {
    uint8_t                 : 5;
    uint8_t sleeptimer_mode : 1;
    uint8_t lowpower_mode   : 1;
    uint8_t                 : 1;
  };
  uint8_t all;
} reg_pmu_low_power_t;
//static_assert(sizeof(reg_pmu_low_power_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_INT_EN_0
typedef union {
  struct {
    uint8_t slope_en_x    : 1;
    uint8_t slope_en_y    : 1;
    uint8_t slope_en_z    : 1;
    uint8_t : 1; // reserved
    uint8_t d_tap_en      : 1;
    uint8_t s_tap_en      : 1;
    uint8_t orient_en     : 1;
    uint8_t flat_en       : 1;
  };
  uint8_t all;
} reg_int_en_0_t;
//static_assert(sizeof(reg_int_en_0_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_INT_EN_1
typedef union {
  struct {
    uint8_t high_en_x     : 1;    // high-g, x-axis
    uint8_t high_en_y     : 1;    // high-g, y-axis
    uint8_t high_en_z     : 1;    // high-g, z-axis
    uint8_t low_en        : 1;    // low-g int
    uint8_t data_en       : 1;    // data ready int
    uint8_t int_ffull_en  : 1;    // fifo full int
    uint8_t int_fwm_en    : 1;    // fifo watermark int
    uint8_t               : 1;    // reserved
  };
  uint8_t all;
} reg_int_en_1_t;
//static_assert(sizeof(reg_int_en_1_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_INT_EN_2
typedef union {
  struct {
    uint8_t slo_no_mot_en_x   : 1;
    uint8_t slo_no_mot_en_y   : 1;
    uint8_t slo_no_mot_en_z   : 1;
    uint8_t slo_no_mot_sel    : 1;  // 0: slow motion, 1: no motion
    uint8_t : 4;
  };
  uint8_t all;
} reg_int_en_2_t;
//static_assert(sizeof(reg_int_en_2_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_INT_MAP_0
typedef union {
  struct {
    uint8_t int1_low        : 1;
    uint8_t int1_high       : 1;
    uint8_t int1_slope      : 1;
    uint8_t int1_slo_no_mot : 1;
    uint8_t int1_d_tap      : 1;
    uint8_t int1_s_tap      : 1;
    uint8_t int1_orient     : 1;
    uint8_t int1_flat       : 1;
  };
  uint8_t all;
} reg_int_map_0_t;
//static_assert(sizeof(reg_int_map_0_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_INT_MAP_1
typedef union {
  struct {
    uint8_t int1_data       : 1;
    uint8_t int1_fwm        : 1;
    uint8_t int1_ffull      : 1;
    uint8_t                 : 1;
    uint8_t                 : 1;
    uint8_t int2_ffull      : 1;
    uint8_t int2_fwm        : 1;
    uint8_t int2_data       : 1;
  };
  uint8_t all;
} reg_int_map_1_t;
//static_assert(sizeof(reg_int_map_1_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_INT_5
typedef union {
  struct {
    uint8_t slope_dur         : 2;  // any motion interrupt duration
    uint8_t slo_no_mot_dur    : 6;  // slow/no-motion duration
  };
  uint8_t all;
} reg_int_5_t;
//static_assert(sizeof(reg_int_5_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_INT_6
typedef union {
  struct {
    uint8_t slope_th          : 8;  // threshold for any motion interrupt
  };
  uint8_t all;
} reg_int_6_t;
//static_assert(sizeof(reg_int_6_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_FIFO_STATUS
typedef union {
  struct {
    uint8_t fifo_frame_counter  : 7;
    uint8_t fifo_overrun        : 1;
  };
  uint8_t all;
} reg_fifo_status_t;
//static_assert(sizeof(reg_fifo_status_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_FIFO_CONFIG_0
typedef union {
  struct {
    uint8_t fifo_water_mark_level_trigger_retain: 6;  // aka, the watermark
    uint8_t : 2;    // reserved
  };
  uint8_t all;
} reg_fifo_config_0_t;
//static_assert(sizeof(reg_fifo_config_0_t) == 1,
//  "Register must be 1 byte");

// REG_ADDR_FIFO_CONFIG_1
typedef union {
  struct {
    uint8_t fifo_data_select    : 2;
    uint8_t : 4;
    uint8_t fifo_mode : 2;
  };
  uint8_t all;
} reg_fifo_config_1_t;
//static_assert(sizeof(reg_fifo_config_1_t) == 1,
//  "Register must be 1 byte");

#define REG_DATA_FIFO_MODE_BYPASS   (0b00)
#define REG_DATA_FIFO_MODE_FIFO     (0b01)
#define REG_DATA_FIFO_MODE_STREAM   (0b10)
#define REG_DATA_FIFO_MODE_RSVD     (0b11)

// Size of the fifo
#define REG_DATA_FIFO_DEPTH         (32)

// REG_ADDR_PMU_RANGE
typedef union {
  struct {
    uint8_t range: 4;
    uint8_t : 4;
  };
  uint8_t all;
} reg_pmu_range_t;
//static_assert(sizeof(reg_pmu_range_t) == 1,
//  "Register must be 1 byte");

#define REG_DATA_PMU_RANGE_2G   (0b0011)
#define REG_DATA_PMU_RANGE_4G   (0b0101)
#define REG_DATA_PMU_RANGE_8G   (0b1000)
#define REG_DATA_PMU_RANGE_16G  (0b1100)

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif // BMA253_REGS_H
