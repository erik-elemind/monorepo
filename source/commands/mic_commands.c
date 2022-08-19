/*
 * mic_commands.c
 *
 *  Created on: May 15, 2022
 *      Author: DavidWang
 */

#include <stdio.h>

#include "mic_commands.h"
#include "system_monitor.h"
#include "command_helpers.h"
#include "adc.h"

void
mic_read_once_command(int argc, char **argv) {
  int32_t mems = system_monitor_event_mic_read_once();
  printf("MEMS reading: %li\n", mems);
}

void
mic_start_sample_command(int argc, char **argv){
  CHK_ARGC(1,2); // allow 0 or 1 arguments

  uint32_t sample_period_ms = MIC_TIMER_MS;
  if(argc==2 && parse_uint32_arg(argv[0], argv[1], &sample_period_ms)){
    system_monitor_event_mic_start_sample(sample_period_ms);
  }
}

void
mic_start_thresh_command(int argc, char **argv){
  CHK_ARGC(1,3); // allow 0 or 2 arguments

  uint32_t sample_period_ms = MIC_TIMER_MS;
  uint32_t num_samples = MIC_SAMPLES_BEFORE_SLEEP;
  if(argc==3 &&
      parse_uint32_arg(argv[0], argv[1], &sample_period_ms) &&
      parse_uint32_arg(argv[0], argv[2], &num_samples)){
    system_monitor_event_mic_start_thresh(sample_period_ms, num_samples);
  }
}

void
mic_stop_command(int argc, char **argv){
  system_monitor_event_mic_stop();
}
