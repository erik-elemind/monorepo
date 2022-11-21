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
#include "ff.h"

FIL hypnogram_log;
void data_log_open_command(int argc, char **argv)
{
  data_log_open();
}

void data_log_close_command(int argc, char **argv)
{
  data_log_close();
}

void hypnogram_log_open_command(int argc, char **argv)
{
	FRESULT result = f_open(&hypnogram_log, "hypnogram_log.txt", FA_CREATE_NEW | FA_WRITE);
	if (result)
	{
		printf("f_open() for %s returned %u\n", "hypnogram_log.txt", result);
		return;
	}
}

void hypnogram_log_close_command(int argc, char **argv)
{
	f_close(&hypnogram_log);
}

void hypnogram_log_write_command(char *data, int len)
{
	UINT bytes_written;
	FRESULT result = f_write(&hypnogram_log, data, len, &bytes_written);
	if (result)
	{
		printf("f_write() for %s returned %u\n", "hypnogram_log.txt", result);
		return;
	}
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
