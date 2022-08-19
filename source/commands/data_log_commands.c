/*
 * data_log_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Oct, 2020
 * Author:  David Wang
 */

#include "data_log_commands.h"
#include "command_helpers.h"
#include "utils.h"
#include "config.h"

void data_log_open_command(int argc, char **argv)
{
  data_log_open();
}

void data_log_close_command(int argc, char **argv)
{
  data_log_close();
}

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

void data_log_compress_command(int argc, char **argv)
{
  CHK_ARGC(2, 2);

  char *name = argv[1];
  data_log_compress(name);
}

void data_log_compress_status_command(int argc, char **argv)
{
  bool is_compressing, result;
  size_t bytes_in_file, bytes_processed;
  data_log_compress_status(&is_compressing, &result, &bytes_in_file, &bytes_processed);

  printf("compression %s (%u of %u bytes). last result: %s.\n", 
    is_compressing ? "in progress" : "done",
    bytes_processed, bytes_in_file,
    result ? "success" : "fail");
}

void data_log_compress_abort_command(int argc, char **argv)
{
  data_log_compress_abort();
}

#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))
