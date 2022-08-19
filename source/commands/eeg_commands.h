/*
 * eeg_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jul, 2020
 * Author:  David Wang
 */

#ifndef EEG_COMMANDS_H_
#define EEG_COMMANDS_H_

#include <stdbool.h>
#include <stdint.h>
#include "eeg_reader.h"

#ifdef __cplusplus
extern "C" {
#endif


void eeg_on_command(int argc, char **argv);
void eeg_off_command(int argc, char **argv);
void eeg_start_command(int argc, char **argv);
void eeg_stop_command(int argc, char **argv);
void eeg_gain_command(int argc, char **argv);
void eeg_info_command(int argc, char **argv);
void eeg_sqw_command(int argc, char **argv);
void eeg_get_gain_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* EEG_COMMANDS_H_ */
