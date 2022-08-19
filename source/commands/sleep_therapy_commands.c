/*
 * eeg_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Oct, 2020
 * Author:  David Wang
 */

#include <stdbool.h>
#include <float.h>
#include "sleep_therapy_commands.h"
#include "command_helpers.h"
#include "loglevels.h"
#include "eeg_processor.h"
#include "eeg_reader.h"
#include "interpreter.h"


//static const char *TAG = "shell"; // Logging prefix for this module

void therapy_start_command(int argc, char **argv)
{
  interpreter_event_start_therapy(THERAPY_TYPE_THERAPY);
}

void therapy_stop_command(int argc, char **argv)
{
  interpreter_event_forced_stop_therapy();
}

void therapy_enable_line_filters_command(int argc, char **argv){
  if (argc != 2) {
//    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get boolean enable
  uint8_t enable;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &enable);
  if (success) {
    eeg_processor_enable_line_filters(enable);
  }
}

void therapy_enable_az_filters_command(int argc, char **argv){
  if (argc != 2) {
//    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get boolean enable
  uint8_t enable;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &enable);
  if (success) {
    eeg_processor_enable_az_filters(enable);
  }
}

void therapy_config_line_filters_command(int argc, char **argv){
  if (argc != 3) {
//    LOGE(TAG, "Error: Command '%s' missing arguments", argv[0]);
    return;
  }

  uint8_t order;
  double cutfreq;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &order);
  success &= parse_double_arg(argv[0], argv[2], &cutfreq);
  if (success) {
    eeg_processor_config_line_filter(order, cutfreq);
  }
}

void therapy_config_az_filters_command(int argc, char **argv){
  if (argc != 3) {
//    LOGE(TAG, "Error: Command '%s' missing arguments", argv[0]);
    return;
  }

  uint8_t order;
  double cutfreq;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &order);
  success &= parse_double_arg(argv[0], argv[2], &cutfreq);
  if (success) {
    eeg_processor_config_az_filter(order, cutfreq);
  }
}

void therapy_start_script_command(int argc, char **argv){
  if (argc != 2) {
//    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }
  interpreter_event_start_script(argv[1]);
}

void therapy_stop_script_command(int argc, char **argv){
  interpreter_event_stop_script(false);
}

void therapy_delay_command(int argc, char **argv){
  if (argc != 2) {
//    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }
  uint32_t time_ms = 0;
#if 0
  bool success = true;
  success &= parse_uint32_arg_min_max(argv[0], argv[1], &order, 100, 86400000UL, &time_ms);
#else
  parse_uint32_arg_min_max(argv[0], argv[1], 100, 86400000UL, &time_ms);
#endif
  interpreter_event_delay(time_ms);
}

void therapy_start_timer1_command(int argc, char **argv){
  if (argc != 2) {
//    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }
  uint32_t time_ms = 0;
#if 0
  bool success = true;
  success &= parse_uint32_arg_min_max(argv[0], argv[1], &order, 100, 86400000UL, &time_ms);
#else
  parse_uint32_arg_min_max(argv[0], argv[1], 100, 86400000UL, &time_ms);
#endif
  interpreter_event_start_timer1(time_ms);
}

void therapy_wait_timer1_command(int argc, char **argv){
  // should have no arguments
  interpreter_event_wait_timer1();
}

void therapy_config_enable_switch_command(int argc, char **argv){
  if (argc != (1+1)) {
//    LOGE("shell", "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get boolean enable
  uint8_t enable;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &enable);
  if (success) {
    eeg_processor_enable_alpha_switch(enable);
  }
}

void therapy_config_alpha_switch_command(int argc, char **argv){
  if (argc != (1+5)) {
//    LOGE("shell", "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  double dur1_sec, dur2_sec, timedLockOut_sec, minRMSPower_uV, alphaThr_dB;
  parse_double_arg_min_max(argv[0], argv[1], 0, DBL_MAX, &dur1_sec);
  parse_double_arg_min_max(argv[0], argv[2], 0, DBL_MAX, &dur2_sec);
  parse_double_arg_min_max(argv[0], argv[3], 0, DBL_MAX, &timedLockOut_sec);
  parse_double_arg_min_max(argv[0], argv[4], 0, DBL_MAX, &minRMSPower_uV);
  parse_double_arg_min_max(argv[0], argv[5], 0, DBL_MAX, &alphaThr_dB);

  eeg_processor_config_alpha_switch((float)dur1_sec, (float)dur2_sec, (float)timedLockOut_sec, (float)minRMSPower_uV, (float)alphaThr_dB);
}

void echt_config_command(int argc, char **argv){
  assert(false);
  //void eeg_processor_config_echt(int fftSize, int filtOrder, float centerFreq, float lowFreq, float highFreq, float inputScale, float sampFreqWithDrop);
  // TODO: implement!
}

void echt_config_simple_command(int argc, char **argv){
  //void eeg_processor_config_echt_simple(float center_freq);
  double center_freq;
  parse_double_arg( argv[0], argv[1], &center_freq );
  eeg_processor_config_echt_simple ( center_freq );
}

void echt_set_channel_command(int argc, char **argv){
  //void eeg_processor_set_echt_channel(eeg_channel_t channel_number);
  eeg_channel_t channel_number = 0;
  // TODO: handle success return value
  parse_uint8_arg(argv[0], argv[1], &channel_number);
  eeg_processor_set_echt_channel(channel_number);
}

void echt_set_min_max_phase_command(int argc, char **argv){
  if (argc != 3) {
//    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }
  //void eeg_processor_set_echt_min_max_phase(float min_phase_rad, float max_phase_rad);
  double min_phase_deg = 0;
  double max_phase_deg = 0;
  double deg2rad_multiplier = 0.01745329251;
  // TODO: handle success return value
  parse_double_arg_min_max(argv[0], argv[1], DBL_MIN, DBL_MAX, &min_phase_deg);
  parse_double_arg_min_max(argv[0], argv[2], DBL_MIN, DBL_MAX, &max_phase_deg);
  eeg_processor_set_echt_min_max_phase(min_phase_deg*deg2rad_multiplier, max_phase_deg*deg2rad_multiplier);
}

void echt_start_command(int argc, char **argv){
  //void eeg_processor_start_echt();
  eeg_processor_start_echt();
}

void echt_stop_command(int argc, char **argv){
  //void eeg_processor_stop_echt();
  eeg_processor_stop_echt();
}













