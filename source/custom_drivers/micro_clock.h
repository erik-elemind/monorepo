/*
 * micro_clock.h
 *
 *  Created on: Nov 5, 2020
 *      Author: David Wang
 */

#ifndef CUSTOM_DRIVERS_MICRO_CLOCK_H_
#define CUSTOM_DRIVERS_MICRO_CLOCK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_micro_clock(void);

void deinit_micro_clock(void);

uint64_t micros(void);

uint64_t millis(void);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_DRIVERS_MICRO_CLOCK_H_ */
