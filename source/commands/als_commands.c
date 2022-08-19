
#include <stdio.h>

#include "als_commands.h"
#include "als.h"
#include "system_monitor.h"
#include "command_helpers.h"


void
als_read_once_command(int argc, char **argv) {
  float lux;
  als_start();
  als_wait();
  als_get_lux(&lux);
  printf("ALS lux: %f\n\r", lux);
  als_stop();
}

void
als_start_sample_command(int argc, char **argv){
  CHK_ARGC(1,2); // allow 0 or 1 arguments

  uint32_t sample_period_ms = ALS_TIMER_MS;
  if(argc==2 && parse_uint32_arg(argv[0], argv[1], &sample_period_ms)){
    system_monitor_event_als_start_sample(sample_period_ms);
  }
}

void
als_stop_command(int argc, char **argv){
  system_monitor_event_als_stop();
}

