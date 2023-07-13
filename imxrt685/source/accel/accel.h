/*
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Sept, 2021
 * Author:  Paul Adelsbach
 *
 * Description: Accelerometer task
 *
 */

#ifndef ACCEL_H
#define ACCEL_H

#include "i2c.h"

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void accel_pretask_init(i2c_rtos_handle_t* i2c_handle);
void accel_task(void *ignored);

// Send various event types to this task:
void accel_turn_off(void);
void accel_start_sample(void);
void accel_start_motion_detect(void);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif  // ACCEL_H
