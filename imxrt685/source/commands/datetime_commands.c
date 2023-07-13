/*
 * datetime_commands.c
 *
 *  Created on: Jun 1, 2021
 *      Author: DavidWang
 */

#include "datetime_commands.h"
#include "loglevels.h"
#include "data_log.h"
#include "rtc.h"

static const char* TAG = "datetime_commands";

void datetime_get_command(int argc, char **argv) {
  uint32_t time_u32 = rtc_get();

  printf("Time: %lu\n\r", time_u32);
}

void datetime_update_command(int argc, char **argv){
  int res;
  
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  uint32_t time_u32 = strtoul(argv[1], NULL, 10);
  res = rtc_set(time_u32);
  if (res) {
    LOGE(TAG, "Error setting time");
    return;
  }
  datetime_get_command(0,NULL);
}

void datetime_human_update_command(int argc, char **argv){
  if (argc != 2) {
//    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  data_log_set_time(argv[1], strlen(argv[1]));
}
