/*
 * eeg_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jul, 2020
 * Author:  David Wang
 */

#include "eeg_commands.h"
#include "loglevels.h"
#include "command_helpers.h"
#include "eeg_sqw.h"

void eeg_on_command(int argc, char **argv)
{
  eeg_reader_event_power_on();
}

void eeg_off_command(int argc, char **argv)
{
  eeg_reader_event_power_off();
}

void eeg_start_command(int argc, char **argv)
{
  eeg_reader_event_start();
}

void eeg_stop_command(int argc, char **argv)
{
  eeg_reader_event_stop();
}

void eeg_gain_command(int argc, char **argv){
  CHK_ARGC(2,2); // allow 1 arguments

  uint8_t gain;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &gain);

  if(success){
    eeg_reader_event_set_ads_gain( gain );
  }
}

void eeg_info_command(int argc, char **argv)
{
  eeg_reader_event_get_info();
}

void eeg_sqw_command(int argc, char **argv) {
  CHK_ARGC(3,3);
  uint8_t freq;
  uint8_t duty_cycle;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &freq);
  success &= parse_uint8_arg(argv[0], argv[2], &duty_cycle);
  if (success) {
    set_eeg_sqw(freq, duty_cycle);
  }
}

void eeg_get_gain_command(int argc, char **argv) {
  float gain = eeg_reader_get_total_gain();
  LOGV("data_log_eeg_gain","%f", gain);
}



