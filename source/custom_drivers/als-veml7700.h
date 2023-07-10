/*
 * als-veml7400.h
 *
 *  Created on: Jun 8, 2023
 *      Author: tyler
 */

#ifndef ALS_VEML7700_H_
#define ALS_VEML7700_H_

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALS_VEML7700_ADDR 0x10

#define VEML7700_REG_CONTROL_REG_0 	0x00
#define VEML7700_REG_PSM		   	0x03
#define VEML7700_REG_ALS_DATA  		0x04

typedef union{
	struct {
		// lsb
		uint16_t als_sd     : 1; // w
		uint16_t als_int_en : 1; // w
		uint16_t _res3      : 2; // w
		uint16_t als_pers   : 2; // w
		uint16_t als_int    : 4; // w
		uint16_t _res2      : 1; // w
		uint16_t als_sm 	: 2; // w
		uint16_t _res       : 3; // w
		// msb
	};
	uint16_t raw;
} veml7700_configuration_register_0;
static_assert(sizeof(veml7700_configuration_register_0) == 2, "Register must be 2 byte");

typedef union{
	struct {
		// lsb
		uint16_t psm_en    : 1; // w
		uint16_t psm 	   : 2; // w
		uint16_t _res2     : 13; // w
		// msb
	};
	uint16_t raw;
} veml7700_psm_register;
static_assert(sizeof(veml7700_psm_register) == 2, "Register must be 2 byte");

typedef union{
	struct {
		// lsb
		uint16_t als_lsb : 8; // r
		uint16_t als_msb : 8; // r
		// msb
	};
	uint16_t raw;
} veml7700_als_data_register;
static_assert(sizeof(veml7700_als_data_register) == 2, "Register must be 2 byte");


// Routines implemented by the caller which perform read and write operations
// using the system-specific APIs.
// These are blocking functions and return 0 for success and non-0 for failure.
typedef int (*als_veml7700_reg_read_func_t)(uint8_t reg_addr, uint16_t* data);
typedef int (*als_veml7700_reg_write_func_t)(uint8_t reg_addr, uint16_t* data);

typedef enum {
ALS_VEML7700_STATUS_SUCCESS = 0,
ALS_VEML7700_STATUS_ERROR,
} als_veml7700_status_t;

als_veml7700_status_t als_veml7700_init(als_veml7700_reg_read_func_t read_func, als_veml7700_reg_write_func_t write_func);
als_veml7700_status_t als_veml7700_power_on(void);
als_veml7700_status_t als_veml7700_power_off(void);
als_veml7700_status_t als_veml7700_read_lux(float *lux);
als_veml7700_status_t als_veml7700_print_debug(void);


// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif
#endif /* ALS_VEML7700_H_ */
