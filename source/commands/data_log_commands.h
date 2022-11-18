/*
 * data_log_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Oct, 2020
 * Author:  David Wang
 */

#ifndef SHELL_DATA_LOG_COMMANDS_H_
#define SHELL_DATA_LOG_COMMANDS_H_

#include "data_log.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

void data_log_open_command(int argc, char **argv);
void data_log_close_command(int argc, char **argv);
void hypnogram_log_open_command(int argc, char **argv);
void hypnogram_log_close_command(int argc, char **argv);
void hypnogram_log_write_command(int argc, char **argv);

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))
void data_log_compress_command(int argc, char **argv);
void data_log_compress_status_command(int argc, char **argv);
void data_log_compress_abort_command(int argc, char **argv);
#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

#ifdef __cplusplus
}
#endif

#endif /* SHELL_DATA_LOG_COMMANDS_H_ */
