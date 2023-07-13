/*
 * amp.h
 *
 *  Created on: Jan 4, 2023
 *      Author: Tyler Gage
 */

#ifndef AMP_H_
#define AMP_H_

#include "config.h"
#include "ssm2518.h"
#include "ssm2529.h"
#include "fsl_i2c_freertos.h"

#ifdef __cplusplus
extern "C" {
#endif

void amp_init(i2c_rtos_handle_t *i2c_handle);
void amp_config();
void amp_mute(bool mute);
void amp_set_volume(uint8_t volume);
void amp_power(bool on);

#ifdef __cplusplus
}
#endif

#endif /* AMP_H_ */
