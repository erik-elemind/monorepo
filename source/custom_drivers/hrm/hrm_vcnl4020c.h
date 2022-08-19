/*
 * hrm_vcnl4020c.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: March, 2020
 * Author:  David Wang
 *
 * Description: Generic driver for the VCNL4020C Heart Rate Monitor chip.
*/
#ifndef HRM_VCNL4020C_H_INC
#define HRM_VCNL4020C_H_INC

#include <stdint.h>

#include "vcnl4020c.h"

#define HRM_BUF_SIZE 256

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

// Temporary store for values from ISR.
typedef struct {
  uint16_t als;
  uint16_t bs;
  uint16_t lo;
  uint16_t hi;
} hrm_isr_vals;

typedef struct {
	vcnl4020c*   sensor;
    uint16_t     size;
    uint16_t     fill;
	uint16_t     red[HRM_BUF_SIZE];
    uint16_t     ir[HRM_BUF_SIZE];
    uint16_t     new_index;
    int64_t      red_total;
    int64_t      ir_total;
    float        spo2_point;
    float        spo2_slope;
    uint8_t      red_curr_regval;
    uint8_t      ir_curr_regval;
    float        hys_pct;
	uint16_t     red_dc;
	int16_t      red_ac;
	uint16_t     ir_dc;
	int16_t      ir_ac;
	hrm_isr_vals isr_vals;
} heart_rate_monitor;

void hrm_init(heart_rate_monitor* hrm, vcnl4020c* vcnl, float hysteresis_percent, float spo2_point, float spo2_slope);
void hrm_reinit(heart_rate_monitor* hrm);
void hrm_sample_red(heart_rate_monitor* hrm);
void hrm_sample_ir(heart_rate_monitor* hrm);
void hrm_sample_ambient(heart_rate_monitor* hrm);
void hrm_update(heart_rate_monitor* hrm, uint16_t red, uint16_t ir);
float hrm_get_heart_rate(heart_rate_monitor* hrm);
float hrm_get_spo2(heart_rate_monitor* hrm);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif //HRM_VCNL4020C_H_INC
