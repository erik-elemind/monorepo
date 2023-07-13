/*
 * als-veml7700.c
 *
 *  Created on: Jun 8, 2023
 *      Author: tyler
 */

#include "als-veml7700.h"

typedef struct {
	als_veml7700_reg_read_func_t read_func;
	als_veml7700_reg_write_func_t write_func;
} als_veml7700_context;

static als_veml7700_context g_context;

als_veml7700_status_t als_veml7700_init(als_veml7700_reg_read_func_t read_func, als_veml7700_reg_write_func_t write_func)
{
	veml7700_configuration_register_0 cfg_reg;
	veml7700_psm_register psm_reg;

	cfg_reg.raw= 0x0000;
	psm_reg.raw= 0x0000;

	g_context.read_func = read_func;
	g_context.write_func = write_func;

	// Configuration Register 0 (RegAddr 0):
	// 0000000000000001
	// XXX************* Reserved
	// ***00*********** ALS Sensitivity: 1X
	// *****X********** Reserved
	// ******0000****** ALS Integration Time: 100 ms
	// **********00**** ALS persistence protect number setting: 1
	// ************XX** Reserved
	// **************0* ALS INT Enable: 0
	// ***************1 ALS Shut Down: Shut Down
	cfg_reg.raw= 0x0001;
	if(g_context.write_func(VEML7700_REG_CONTROL_REG_0, &cfg_reg.raw) != kStatus_Success)
	{
		return ALS_VEML7700_STATUS_ERROR;
	}

	// Power Saving Mode Register (RegAddr 3):
	// 0000000000000001
	// XXXXXXXXXXXXX*** Reserved
	// *************00* Power Saving Mode: Mode 1
	// ***************1 Power Saving Mode Enable: Enabled
	psm_reg.raw= 0x0001;
	if(g_context.write_func(VEML7700_REG_PSM, &psm_reg.raw) != kStatus_Success)
	{
		return ALS_VEML7700_STATUS_ERROR;
	}

	return ALS_VEML7700_STATUS_SUCCESS;
}

als_veml7700_status_t als_veml7700_power_on(void)
{
	veml7700_configuration_register_0 cfg_reg;

	if(g_context.read_func(VEML7700_REG_CONTROL_REG_0, &cfg_reg.raw) != kStatus_Success)
	{
		return ALS_VEML7700_STATUS_ERROR;
	}

	// Turn on veml7700
	cfg_reg.als_sd = 0;

	if(g_context.write_func(VEML7700_REG_CONTROL_REG_0, &cfg_reg.raw) != kStatus_Success)
	{
		return ALS_VEML7700_STATUS_ERROR;
	}

	return ALS_VEML7700_STATUS_SUCCESS;
}

als_veml7700_status_t als_veml7700_power_off(void)
{
	veml7700_configuration_register_0 cfg_reg;

	if(g_context.read_func(VEML7700_REG_CONTROL_REG_0, &cfg_reg.raw) != kStatus_Success)
	{
		return ALS_VEML7700_STATUS_ERROR;
	}

	// Turn off veml7700
	cfg_reg.als_sd = 1;

	if(g_context.write_func(VEML7700_REG_CONTROL_REG_0, &cfg_reg.raw) != kStatus_Success)
	{
		return ALS_VEML7700_STATUS_ERROR;
	}

	return ALS_VEML7700_STATUS_SUCCESS;
}

als_veml7700_status_t als_veml7700_read_lux(float *lux)
{
	veml7700_als_data_register als_sample_data;
	const float lux_per_bit = 0.0576;

	if(g_context.read_func(VEML7700_REG_ALS_DATA, &als_sample_data.raw) != kStatus_Success)
	{
		return ALS_VEML7700_STATUS_ERROR;
	}

	*lux = (float) ((float) als_sample_data.raw * lux_per_bit);

	return ALS_VEML7700_STATUS_SUCCESS;
}

als_veml7700_status_t als_veml7700_print_debug(void)
{
	uint16_t data;

	for(uint8_t i=0;i<7;i++)
	{
		if (0 != g_context.read_func(i, &data)) {
			printf("Error reading REG %d\r\n", i);
			return ALS_VEML7700_STATUS_ERROR;
		}

		printf("REG: %2X, %4X\r\n", i, data);
	}

	return ALS_VEML7700_STATUS_SUCCESS;
}



