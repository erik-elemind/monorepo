
#include <stdio.h>

#include "als_commands.h"
#include "als.h"
#include "als-veml7700.h"
#include "system_monitor.h"
#include "command_helpers.h"
#include "util_delay.h"
#include "loglevels.h"

static const char *TAG = "als_commands";	// Logging prefix for this module

void
als_read_once_command(int argc, char **argv) {
	// TODO: Complete ALS driver and implementation of this command.
	LOGW(TAG, "%s command is not yet implemented.", argv[0]);
#if 0
  float lux;
  als_start();
  util_delay_ms(1000); // Delay for 1 second to ensure settling time
  als_get_lux(&lux);
  printf("ALS lux: %f\n\r", lux);
  als_stop();
#endif
}

void
als_start_sample_command(int argc, char **argv){
	system_monitor_event_als_start_sample();
}

void
als_stop_command(int argc, char **argv){
	// TODO: Complete ALS driver and implementation of this command.
	LOGW(TAG, "%s command is not yet implemented.", argv[0]);
#if 0
  system_monitor_event_als_stop();
#endif
}

void
als_test_command(int argc, char **argv){
	als_veml7700_print_debug();
}
