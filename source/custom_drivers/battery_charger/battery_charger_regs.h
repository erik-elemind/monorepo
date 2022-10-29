/*
 * battery_charger.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger
 *
 * Description: BQ25618 Battery Charger I2C register definitions.
 *
 *  Constants (i.e. values for specific fields) are in the source file.
 */
#ifndef BATTERY_CHARGER_REGS_H
#define BATTERY_CHARGER_REGS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

/// Input Current Limit (REG00) register type
typedef union {
  struct {
    // lsb
    uint8_t iindpm : 5; // r/w
    bool batsns_dis : 1; // r/w
    bool ts_ignore : 1; // r/w
    bool en_hiz : 1; // r/w
    // msb
  };
  uint8_t raw;
} input_current_limit_t;
static_assert(sizeof(input_current_limit_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Control 0 (REG01) register type
typedef union {
  struct {
    // lsb
    bool min_vbat_sel : 1; // r/w
    uint8_t sys_min : 3; // r/w
    bool chg_config : 1; // r/w
    bool bst_config : 1; // r/w
    bool wd_rst : 1; // r/w
    bool pfm_dis : 1; // r/w
    // msb
  };
  uint8_t raw;
} charger_control_0_t;
static_assert(sizeof(charger_control_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Current Limit (REG02) register type
typedef union {
  struct {
    // lsb
    uint8_t ichg : 6; // r/w
    bool q1_fullon : 1; // r/w
    bool reserved : 1; // r/w
    // msb
  };
  uint8_t raw;
} charger_current_limit_t;
static_assert(sizeof(charger_current_limit_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Precharge and Termination Current Limit (REG03) register type
typedef union {
  struct {
    // lsb
    uint8_t iterm : 4; // r/w
    uint8_t iprechg : 4; // r/w
    // msb
  };
  uint8_t raw;
} precharge_termination_current_limit_t;
static_assert(sizeof(precharge_termination_current_limit_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Battery Voltage Limit (REG04) register type
typedef union {
  struct {
    // lsb
    bool vrechg : 1; // r/w
    uint8_t topoff_timer : 2; // r/w
    uint8_t vbatreg : 5; // r/w
    // msb
  };
  uint8_t raw;
} battery_voltage_limit_t;
static_assert(sizeof(charger_current_limit_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Control 1 (REG05) register type
typedef union {
  struct {
    // lsb
    bool jeita_vset : 1; // r/w
    bool treg : 1; // r/w
    bool chg_timer : 1; // r/w
    bool en_timer : 1; // r/w
    uint8_t watchdog : 2; // r/w
    bool reserved : 1; // r/w
    bool en_term : 1; // r/w
    // msb
  };
  uint8_t raw;
} charger_control_1_t;
static_assert(sizeof(charger_control_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Control 2 (REG06) register type
typedef union {
  struct {
    // lsb
    uint8_t vindpm : 4; // r/w
    uint8_t boostv : 2; // r/w
    uint8_t ovp : 2; // r/w
    // msb
  };
  uint8_t raw;
} charger_control_2_t;
static_assert(sizeof(charger_control_2_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Control 3 (REG07) register type
typedef union {
  struct {
    // lsb
    uint8_t vindpm_bat_track : 2; // r/w
    bool batfet_rst_en : 1; // r/w
    bool batfet_dly : 1; // r/w
    bool batfet_rst_wvbus : 1; // r/w
    bool batfet_dis : 1; // r/w
    bool tmr2x_en : 1; // r/w
    bool iindet_en : 1; // r/w
    // msb
  };
  uint8_t raw;
} charger_control_3_t;
static_assert(sizeof(charger_control_3_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Status 0 (REG08) register type
typedef union {
  struct {
    // lsb
    bool vsys_stat : 1; // ro
    bool therm_stat : 1; // ro
    bool pg_stat : 1; // ro
    uint8_t chrg_stat : 2; // ro
    uint8_t vbus_stat : 3; // ro
    // msb
  };
  uint8_t raw;
} charger_status_0_t;
static_assert(sizeof(charger_status_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Status 1 (REG09) register type
typedef union {
  struct {
    // lsb
    uint8_t ntc_fault : 3; // ro
    bool bat_fault : 1; // ro
    uint8_t chrg_fault : 2; // ro
    bool boost_fault : 1; // ro
    bool watchdog_fault : 1; // ro
    // msb
  };
  uint8_t raw;
} charger_status_1_t;
static_assert(sizeof(charger_status_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Status 2 (REG0A) register type
typedef union {
  struct {
    // lsb
    bool iindpm_int_mask : 1; // r/w
    bool windpm_int_mask : 1; // r/w
    bool acov_stat : 1; // ro
    bool topoff_active : 1; // ro
    bool batsns_stat : 1; // ro
    bool iintpm_stat : 1; // ro
    bool vindpm_stat : 1; // ro
    bool vbus_gd : 1; // ro
    // msb
  };
  uint8_t raw;
} charger_status_2_t;
static_assert(sizeof(charger_status_2_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Part Information (REG0B) register type
typedef union {
  struct {
    // lsb
    uint8_t reserved : 3; // ro
    uint8_t pn : 4; // ro
    bool reg_rst : 1; // r/w
    // msb
  };
  uint8_t raw;
} part_information_t;
static_assert(sizeof(part_information_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Control 4 (REG0C) register type
typedef union {
  struct {
    // lsb
    uint8_t jeita_vt3 : 2; // r/w
    uint8_t jeita_vt2 : 2; // r/w
    uint8_t jeita_warm_iset : 2; // r/w
    uint8_t jeita_cool_iset : 2; // r/w
    // msb
  };
  uint8_t raw;
} charger_control_4_t;
static_assert(sizeof(charger_control_4_t) == sizeof(uint8_t),
  "Register must be 1 byte");

#ifdef __cplusplus
}
#endif

#endif  // BATTERY_CHARGER_REGS_H
