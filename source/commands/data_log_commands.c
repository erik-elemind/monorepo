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
#include "data_log.h"
#include "loglevels.h"
#include "utils.h"
#include "config.h"
#include "ff.h"

static FIL user_metrics_log;

void data_log_open_command(int argc, char **argv)
{
  data_log_open();
}

void data_log_close_command(int argc, char **argv)
{
  data_log_close();
}

void user_metrics_log_open_command(void)
{
  user_metrics_log_open(&user_metrics_log);
}

void user_metrics_log_close_command(void)
{
	f_close(&user_metrics_log);
}

void user_metrics_log_write_command(char *data, int len)
{
	UINT bytes_written;
	FRESULT result = f_write(&user_metrics_log, data, len, &bytes_written);
	if (result)
	{
		LOGE("user_metrics", "f_write() for %s returned %u\n", "user_metrics", result);
		return;
	}
	f_sync(&user_metrics_log);
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
