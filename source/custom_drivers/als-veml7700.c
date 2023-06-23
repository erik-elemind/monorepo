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
	g_context.read_func = read_func;
	g_context.write_func = write_func;

	return ALS_VEML7700_STATUS_SUCCESS;
}

als_veml7700_status_t als_veml7700_print_debug(void)
{
	uint8_t data[2];
	uint16_t printVar;

	for(uint8_t i=0;i<7;i++)
	{
		if (0 != g_context.read_func(i, data, 2)) {
			printf("Error reading REG %d\r\n", i);
			return ALS_VEML7700_STATUS_ERROR;
		}
		printVar = 0;
		printVar |= (data[1] << 8);
		printVar |= (data[0]);
		printf("REG: %2X, %4X\r\n", i, printVar);
	}


	data[0] = 0x00;
	data[1] = 0x00;

	if (0 != g_context.write_func(0, data)) {
		printf("Error writing REG %d\r\n", 0);
		return ALS_VEML7700_STATUS_ERROR;
	}

	vTaskDelay(100);

	for(uint8_t i=0;i<7;i++)
	{
		if (0 != g_context.read_func(i, data, 2)) {
			printf("Error reading REG %d\r\n", i);
			return ALS_VEML7700_STATUS_ERROR;
		}
		printVar = 0;
		printVar |= (data[1] << 8);
		printVar |= (data[0]);
		printf("REG: %2X, %4X\r\n", i, printVar);
	}

	return ALS_VEML7700_STATUS_SUCCESS;
}



