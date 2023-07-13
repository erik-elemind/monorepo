#include "accel.h"
#include <stdio.h>

void accel_start_sample_command(int argc, char **argv){
  accel_start_sample();
}

void accel_start_motion_detect_command(int argc, char **argv){
  accel_start_motion_detect();
}

void accel_stop_command(int argc, char **argv) {
  accel_turn_off();
}
