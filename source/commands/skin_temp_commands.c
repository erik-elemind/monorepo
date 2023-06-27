/*
 * skin_temp_commands.c
 *
 *  Created on: May 15, 2022
 *      Author: DavidWang
 */

#include "skin_temp_commands.h"
#include "eeg_reader.h"
#include "loglevels.h"

static const char *TAG = "skin_temp_commands";	// Logging prefix for this module

void
skin_temp_start_sample_command(int argc, char **argv){
	// TODO: Complete SkinTemp driver and implementation of this command.
	LOGW(TAG, "%s command is not yet implemented.", argv[0]);
}

void
skin_temp_stop_command(int argc, char **argv){
	// TODO: Complete SkinTemp driver and implementation of this command.
	LOGW(TAG, "%s command is not yet implemented.", argv[0]);
}
