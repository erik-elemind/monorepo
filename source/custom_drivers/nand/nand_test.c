/*
 * nand_test.c
 *
 *  Created on: Oct 5, 2022
 *      Author: tyler
 */
#include "nand_test.h"

void nand_test1(void)
{
	nand_platform_init();

	//Test 1: Test command response
	uint8_t p_command = 0x9B;
	uint8_t command_len = 1;
	uint32_t* p_data[3] = {0};
	uint32_t data_len = 8;

	nand_platform_command_response(&p_command, command_len, p_data, data_len);

}
