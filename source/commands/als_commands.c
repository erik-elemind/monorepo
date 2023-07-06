
#include <stdio.h>

#include "als_commands.h"
#include "als.h"
#include "als-veml7700.h"
#include "system_monitor.h"
#include "command_helpers.h"


void
als_read_once_command(int argc, char **argv) {
  float lux;
  als_start();
  als_get_lux(&lux);
  printf("ALS lux: %f\n\r", lux);
  als_stop();
}

void
als_start_sample_command(int argc, char **argv){

	system_monitor_event_als_start_sample();
}

void
als_stop_command(int argc, char **argv){
  system_monitor_event_als_stop();
}

void
als_test_command(int argc, char **argv){
	als_veml7700_print_debug();
}
