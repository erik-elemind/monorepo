/*
 * vcnl4020c_regs.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: April, 2020
 * Author:  David Wang
 *
 * Description: Generic driver for the VCNL4020C Heart Rate Monitor chip.
*/
#ifndef VCNL4020C_REGS_H_INC
#define VCNL4020C_REGS_H_INC

#include <stdint.h>

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif


typedef union{
	struct {
		// lsb
		uint8_t selftimed_en : 1; // r/w
		uint8_t bs_en        : 1; // r/w
		uint8_t als_en       : 1; // r/w
		uint8_t bs_od        : 1; // r/w
		uint8_t als_od       : 1; // r/w
		uint8_t bs_data_rdy  : 1; // r
		uint8_t als_data_rdy : 1; // r
		uint8_t config_lock  : 1; // r
		// msb
	};
	uint8_t init_all;
}hrm_command_val;

typedef union{
	struct {
		// lsb
		uint8_t revision_id : 4; // r
		uint8_t product_id  : 4; // r
		// msb
	};
	uint8_t init_all;
}hrm_product_id_val;

typedef union{
	struct {
		// lsb
		uint8_t rate   : 3; // r/w
		uint8_t _na    : 5;
		// msb
	};
	uint8_t init_all;
}hrm_biosensor_config_val;

typedef union{
	struct {
		// lsb
		uint8_t led_current   : 6; // r/w
		uint8_t fuse_prog_id  : 2; // r
		// msb
	};
	uint8_t init_all;
}hrm_led_current_val;


typedef union{
	struct {
		// lsb
		uint8_t avg_func            : 3; // r/w
		uint8_t auto_offset_comp    : 1; // r/w
		uint8_t ambient_sample_rate : 3; // r/w
		uint8_t enable_cont_conv    : 1; // r/w
		// msb
	};
	uint8_t init_all;
}hrm_ambient_config_val;

typedef union{
	struct {
		// lsb
		uint8_t int_thres_sel    : 1; // r/w
		uint8_t int_thres_en     : 1; // r/w
		uint8_t int_als_ready_en : 1; // r/w
		uint8_t int_bs_ready_en  : 1; // r/w
		uint8_t _na              : 1;
		uint8_t int_count_exceed : 3; // r/w
		// msb
	};
	uint8_t init_all;
}hrm_interrupt_config_val;

typedef union{
	struct {
		// lsb
		uint8_t int_th_hi     : 1; // r/w
		uint8_t int_th_lo     : 1; // r/w
		uint8_t int_als_ready : 1; // r/w
		uint8_t int_bs_ready  : 1; // r/w
		uint8_t _na           : 4;
		// msb
	};
	uint8_t init_all;
}hrm_interrupt_status_val;

typedef union{
	struct {
		// lsb
		uint8_t dead_time  : 3; // r/w
		uint8_t freq       : 2; // r/w
		uint8_t delay_time : 3; // r/w
		// msb
	};
	uint8_t init_all;
}hrm_biosensor_timing_val;



// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif // VCNL4020C_REGS_H_INC
