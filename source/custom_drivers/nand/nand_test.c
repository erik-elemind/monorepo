/*
 * nand_test.c
 *
 *  Created on: Oct 5, 2022
 *      Author: tyler
 */
#include "nand_test.h"

void nand_test1(void) {
	status_t status;

	// TEST READ ID
	uint8_t p_mfg_id;
	uint16_t p_device_id;

	status = nand_get_id(NULL, &p_mfg_id, &p_device_id);
	printf("status = %d\r\n", status);
	printf("MFG ID: %02X\r\n", p_mfg_id);
	printf("DEV ID: %04X\r\n", p_device_id);


	uint8_t data;
	// TEST PROTECTION REGISTER READ
	nand_get_feature_reg(NULL, FEATURE_REG_PROTECTION, &data);
	printf("status = %d\r\n", status);
	printf("Status Reg 1: %04X\r\n", data);

	// TEST CONFIGURATION REGISTER READ
	nand_get_feature_reg(NULL, FEATURE_REG_CONFIGURATION, &data);
	printf("status = %d\r\n", status);
	printf("Status Reg 1: %04X\r\n", data);

	// TEST STATUS REGISTER READ
	nand_get_feature_reg(NULL, FEATURE_REG_STATUS, &data);
	printf("status = %d\r\n", status);
	printf("Status Reg 1: %04X\r\n", data);

	// TEST PRINT DETAILED STATUS
	nand_print_detailed_status(NULL);


}
