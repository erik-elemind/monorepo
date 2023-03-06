/*
 * battery_charger_regs_BQ25887.h
 *
 *  Created on: Oct 28, 2022
 *      Author: Tyler Gage
 */

#ifndef BATTERY_CHARGER_BATTERY_CHARGER_REGS_BQ25887_H_
#define BATTERY_CHARGER_BATTERY_CHARGER_REGS_BQ25887_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

/// Part Information (REG 0x25) register type
typedef union {
  struct {
    // lsb
    uint8_t dev_rev : 3; // ro
    uint8_t pn : 4; // ro
    bool reg_rst : 1; // r/w
    // msb
  };
  uint8_t raw;
} part_information_t;
static_assert(sizeof(part_information_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Control 1 (REG 0x05) register type
typedef union {
  struct {
    // lsb
	bool tmr2x_en : 1; // r/w
    uint8_t chg_timer : 2; // r/w
    bool en_timer : 1; // r/w
    uint8_t watchdog : 2; // r/w
    bool stat_dis : 1; // r/w
    bool en_term : 1; // r/w
    // msb
  };
  uint8_t raw;
} charger_control_1_t;
static_assert(sizeof(charger_control_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Control 3 (REG 0x07) register type
typedef union {
  struct {
    // lsb
    uint8_t res : 4; // ro
    uint8_t topoff_timer : 2; // r/w
    bool wd_rst : 1; // r/w
    bool pfm_dis : 1; // r/w
    // msb
  };
  uint8_t raw;
} charger_control_3_t;
static_assert(sizeof(charger_control_3_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Status 1 (REG 0x0B) register type
typedef union {
  struct {
    // lsb
    uint8_t chrg_status : 3; // ro
    bool wd_stat : 1; // ro
    bool treg_stat : 1; // ro
    bool vindpm_stat : 1; // ro
    bool iindpm_stat : 1; // ro
    bool res : 1; // ro
    // msb
  };
  uint8_t raw;
} charger_status_1_t;
static_assert(sizeof(charger_status_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Charger Status 2 (REG 0x0C) register type
typedef union {
  struct {
    // lsb
    bool res0: 1; // ro
    uint8_t ico_stat : 2; // ro
    bool res1 : 1; // ro
    uint8_t vbus_stat : 3; // ro
    bool pg_stat : 1; // ro
    // msb
  };
  uint8_t raw;
} charger_status_2_t;
static_assert(sizeof(charger_status_2_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// NTC Status (REG 0x0D) register type
typedef union {
  struct {
    // lsb
    uint8_t ts_stat : 3; // ro
    uint8_t res : 5; // ro
    // msb
  };
  uint8_t raw;
} ntc_status_t;
static_assert(sizeof(ntc_status_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// Fault Status (REG 0x0E) register type
typedef union {
  struct {
    // lsb
    uint8_t res1 : 4; // ro
    bool tmr_stat : 1; // ro
    bool res2 : 1; // ro
    bool tshut_stat : 1; // ro
    bool vbus_ovp_stat : 1; // ro
    // msb
  };
  uint8_t raw;
} fault_status_t;
static_assert(sizeof(fault_status_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// ADC Control (REG 0x15) register type
typedef union {
  struct {
    // lsb
    uint8_t res : 4; // r
    uint8_t adc_sample : 2; // rw
    bool adc_rate : 1; // rw
    bool adc_en : 1; // rw
    // msb
  };
  uint8_t raw;
} adc_control_t;
static_assert(sizeof(adc_control_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// IBUS ADC 1 (REG 0x17) register type
typedef union {
  struct {
    // lsb
    uint8_t ibus_adc : 8; // r
    // msb
  };
  uint8_t raw;
} ibus_adc_1_t;
static_assert(sizeof(ibus_adc_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// IBUS ADC 0 (REG 0x18) register type
typedef union {
  struct {
    // lsb
    uint8_t ibus_adc : 8; // r
    // msb
  };
  uint8_t raw;
} ibus_adc_0_t;
static_assert(sizeof(ibus_adc_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// ICHG ADC 1 (REG 0x19) register type
typedef union {
  struct {
    // lsb
    uint8_t ichg_adc : 8; // r
    // msb
  };
  uint8_t raw;
} ichg_adc_1_t;
static_assert(sizeof(ichg_adc_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// ICHG ADC 0 (REG 0x1A) register type
typedef union {
  struct {
    // lsb
    uint8_t ichg_adc : 8; // r
    // msb
  };
  uint8_t raw;
} ichg_adc_0_t;
static_assert(sizeof(ichg_adc_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VBUS ADC 1 (REG 0x1B) register type
typedef union {
  struct {
    // lsb
    uint8_t vbus_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vbus_adc_1_t;
static_assert(sizeof(vbus_adc_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VBUS ADC 0 (REG 0x1C) register type
typedef union {
  struct {
    // lsb
    uint8_t vbus_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vbus_adc_0_t;
static_assert(sizeof(vbus_adc_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VBAT ADC 1 (REG 0x1D) register type
typedef union {
  struct {
    // lsb
    uint8_t vbat_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vbat_adc_1_t;
static_assert(sizeof(vbat_adc_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VBAT ADC 0 (REG 0x1E) register type
typedef union {
  struct {
    // lsb
    uint8_t vbat_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vbat_adc_0_t;
static_assert(sizeof(vbat_adc_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VCELLTOP ADC 1 (REG 0x1F) register type
typedef union {
  struct {
    // lsb
    uint8_t vcelltop_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vcelltop_adc_1_t;
static_assert(sizeof(vcelltop_adc_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VCELLTOP ADC 0 (REG 0x20) register type
typedef union {
  struct {
    // lsb
    uint8_t vcelltop_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vcelltop_adc_0_t;
static_assert(sizeof(vcelltop_adc_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VCELLBOT ADC 1 (REG 0x26) register type
typedef union {
  struct {
    // lsb
    uint8_t vcellbot_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vcellbot_adc_1_t;
static_assert(sizeof(vcellbot_adc_1_t) == sizeof(uint8_t),
  "Register must be 1 byte");

/// VCELLBOT ADC 0 (REG 0x27) register type
typedef union {
  struct {
    // lsb
    uint8_t vcellbot_adc : 8; // r
    // msb
  };
  uint8_t raw;
} vcellbot_adc_0_t;
static_assert(sizeof(vcellbot_adc_0_t) == sizeof(uint8_t),
  "Register must be 1 byte");

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_CHARGER_BATTERY_CHARGER_REGS_BQ25887_H_ */
