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

#ifdef __cplusplus
extern "C" {
#endif

#define ALS_VEML7700_ADDR 0x10

// Routines implemented by the caller which perform read and write operations
// using the system-specific APIs.
// These are blocking functions and return 0 for success and non-0 for failure.
typedef int (*als_veml7700_reg_read_func_t)(uint8_t reg_addr, uint8_t* data, uint8_t len);
typedef int (*als_veml7700_reg_write_func_t)(uint8_t reg_addr, uint8_t* data);

typedef enum {
ALS_VEML7700_STATUS_SUCCESS = 0,
ALS_VEML7700_STATUS_ERROR,
} als_veml7700_status_t;

als_veml7700_status_t als_veml7700_init(als_veml7700_reg_read_func_t read_func, als_veml7700_reg_write_func_t write_func);


// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif
#endif /* ALS_VEML7700_H_ */
