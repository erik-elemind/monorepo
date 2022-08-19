/*
 * vcnl4020c.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: March, 2020
 * Author:  David Wang
 *
 * Description: Generic driver for the VCNL4020C Heart Rate Monitor chip.
*/
#ifndef VCNL4020C_H_INC
#define VCNL4020C_H_INC

#include <stdint.h>
#include <stdbool.h>

#include "fsl_pint.h"
#include "fsl_i2c_freertos.h"

#include "vcnl4020c_regs.h"


// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif


/*
 * VCNL status type.
 * Used to indicate the return status of methods.
 */
typedef uint8_t vcnl_status;
#define VCNL_STATUS_SUCCESS 0
#define VCNL_STATUS_FAIL 1


typedef struct vcnl4020c{
    i2c_rtos_handle_t* master_rtos_handle;
    uint8_t address;
    uint8_t red_port;
    uint8_t red_pin;
    uint8_t ir_port;
    uint8_t ir_pin;
} vcnl4020c;


void vcnl4020c_init(vcnl4020c* hrm, i2c_rtos_handle_t* master_rtos_handle,
		uint8_t red_port, uint8_t red_pin,
		uint8_t ir_port, uint8_t ir_pin,
		pint_pin_int_t pint_pin, pint_cb_t pint_callback, bool config_pint_interrupt);

hrm_product_id_val vcnl4020c_get_product_id(vcnl4020c* hrm);
void        vcnl4020c_set_measure_once(vcnl4020c* hrm, bool enable_ambient, bool enable_biosensor);
void        vcnl4020c_set_measure_periodic(vcnl4020c* hrm, bool enable_ambient, bool enable_biosensor);
void        vcnl4020c_set_biosensor_rate(vcnl4020c* hrm, uint8_t rate);
void        vcnl4020c_set_led_current(vcnl4020c* hrm, uint8_t current);
void        vcnl4020c_set_ambient_config(vcnl4020c* vcnl, bool enable_cont_conv, uint8_t ambient_sample_rate,
                                         bool enable_offset_comp, uint8_t samples_to_average);
void        vcnl4020c_set_interrupt(vcnl4020c* hrm, bool on_biosensor_ready, bool on_ambient_ready,
		                            bool enable_thres, bool sel_thres, uint8_t thres_count);
void        vcnl4020c_set_biosensor_timing(vcnl4020c* hrm, uint8_t delay_time, uint8_t freq, uint8_t dead_time);
hrm_interrupt_status_val vcnl4020c_get_interrupt_status(vcnl4020c* vcnl);
void        vcnl4020c_clr_interrupt_status(vcnl4020c* vcnl);
uint16_t    vcnl4020c_get_ambient(vcnl4020c* hrm);
uint16_t    vcnl4020c_get_lo_thres(vcnl4020c* hrm);
vcnl_status vcnl4020c_set_lo_thres(vcnl4020c* hrm, uint16_t val);
uint16_t    vcnl4020c_get_hi_thres(vcnl4020c* hrm);
vcnl_status vcnl4020c_set_hi_thres(vcnl4020c* hrm, uint16_t val);
uint16_t    vcnl4020c_get_biosensor(vcnl4020c* hrm);
void        vcnl4020c_set_led(vcnl4020c* vcnl, bool red, bool ir);


// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif // VCNL4020C_H_INC
