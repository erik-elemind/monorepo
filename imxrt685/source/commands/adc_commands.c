/*
 * adc_commands.c
 *
 *  Created on: Jun 7, 2023
 *      Author: tyler
 */
#include "adc_commands.h"
#include "command_helpers.h"

void adc_read_command(int argc, char **argv)
{
	CHK_ARGC(2, 2);
	int channel = atoi(argv[1]);
	adc_read(channel);
}
