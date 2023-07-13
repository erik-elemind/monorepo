/*
 * eeg_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Oct, 2020
 * Author:  David Wang
 */

#ifndef __SLEEP_THERAPY_H__
#define __SLEEP_THERAPY_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


void therapy_start_command(int argc, char **argv);
void therapy_stop_command(int argc, char **argv);

void therapy_enable_line_filters_command(int argc, char **argv);
void therapy_enable_az_filters_command(int argc, char **argv);

void therapy_config_line_filters_command(int argc, char **argv);
void therapy_config_az_filters_command(int argc, char **argv);

void therapy_start_script_command(int argc, char **argv);
void therapy_stop_script_command(int argc, char **argv);

void therapy_delay_command(int argc, char **argv);
void therapy_start_timer1_command(int argc, char **argv);
void therapy_wait_timer1_command(int argc, char **argv);

void therapy_config_enable_switch_command(int argc, char **argv);
void therapy_config_alpha_switch_command(int argc, char **argv);

void echt_config_command(int argc, char **argv);
void echt_config_simple_command(int argc, char **argv);
void echt_set_channel_command(int argc, char **argv);
void echt_set_min_max_phase_command(int argc, char **argv);
void echt_start_command(int argc, char **argv);
void echt_stop_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* __SLEEP_THERAPY_H__ */
