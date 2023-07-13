/*
 * dhara_commands.c
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Shell commands for UFFS.
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"

#include "command_helpers.h"
#include "dhara_utils.h"
#include "map.h"
#include "nand.h"

#include "board_config.h"

#include "loglevels.h"

static const char* TAG = "dhara_cmd";

#define DHARA_TESTS_USE_MALLOC 1

void
dhara_write_read_command(int argc, char **argv)
{
  int result;
  dhara_error_t err;

  int sector_count = 2000;  // number of pages
  int sector;

  struct dhara_map *map = dhara_get_my_map();
  uint16_t sector_size = dhara_map_sector_size_bytes(map);

  uint32_t data_size = sector_size;

#ifdef DHARA_TESTS_USE_MALLOC  // use static ram if your heap is low
  uint8_t *data = malloc(data_size);
  uint8_t *data_out = malloc(data_size);

  if ((data == NULL) || (data_out == NULL)) {
    LOGE("dhara_write_read", "malloc failure, returned NULL");
  }
#else
  // If these are not static, you'll need a huge Task stack size to push them.
  static uint8_t data[NAND_PAGE_PLUS_SPARE_SIZE];
  static uint8_t data_out[NAND_PAGE_PLUS_SPARE_SIZE];
#endif

  uint32_t index;

  // Populate some non-zero values:
  for (index = 0 ; index < data_size; index++) {
      data[index] = index & 0xFF;
  }

  // A marker value:
  for (index = 0 ; index < data_size; index++) {
      data_out[index] = 0xa5;
  }

  // Reduce logging to accurately measure speed
  int log_level_orig = g_runtime_log_level;
  g_runtime_log_level = 1;

  TickType_t t0 = xTaskGetTickCount();

  int run_count = 0;
  for (run_count = 0 ; run_count < 1; run_count++) {
    for (sector = 0 ; sector < sector_count; sector++) {

      if (sector % (64*64) == 0) printf("\n");

      //printf("Write to %d:%d:%d (%d B): \n", (int)sector, (int)page, (int)page_offset, (int)data_size);
      result = dhara_map_write(map, (dhara_sector_t)sector, (uint8_t *)data, &err);
      if (result != 0) {
        LOGE(TAG, "dhara_map_write(): result: %d, err: %d, sector %d", (int)result, (int)err, (int)sector);
        goto dhara_write_read_exit;
      }
      //printf(".");
      if (sector % 64 == 0)printf(".");
      fflush(stdout);

    }  // sectors


    // Print write speed (in bytes/s)
    {
      TickType_t t1 = xTaskGetTickCount();
      TickType_t delta_ticks = t1 - t0;
      uint32_t delta_ms = portTICK_PERIOD_MS*delta_ticks;
      uint32_t bytes = sector_size * sector_count;  // TODO: Use a variable for number of blocks
      float kbytes = bytes/1024.0;
      float kbytes_per_s = (float)bytes/delta_ms;
      printf("\n");
      printf("Write %0.1f KB at %0.3f KB/s\n", kbytes, kbytes_per_s);
    }

    t0 = xTaskGetTickCount();

    printf("\nEntering read-only loop:\n");
      printf("\n");
      printf("\n");
      printf("\n");
      printf("\n");
    fflush(stdout);

    for (sector = 0 ; sector < sector_count; sector++) {
      if (sector % (64*64) == 0)printf("\n");

      result = dhara_map_read(map, (dhara_sector_t)sector, data_out, &err);
      if (result != 0) {
        LOGE(TAG, "dhara_map_read(): result: %d, err: %d, sector %d", (int)result, (int)err, (int)sector);
        goto dhara_write_read_exit;
      }

      // If the memory is different, count it as an error:
      if (result == 0) {
        int result_cmp = memcmp(data, data_out, data_size);
        if (result_cmp != 0) {
          LOGE("dhara_write_read", "status==0 but memcmp result was: %d for sector %d", result_cmp, sector);
          printf("\n");

          // Print out the data, though it is better to use the debugger
          // hex_dump(data_out, data_size, 0);

          fflush(stdout);
          printf("\n");
          goto dhara_write_read_exit;
        }
      }

//      printf(".");
      if (sector % 64 == 0)printf(".");
      fflush(stdout);
    } // end for loop over 'run_count'

    // Print read speed (in bytes/s)
    {
      TickType_t t1 = xTaskGetTickCount();
      TickType_t delta_ticks = t1 - t0;
      uint32_t delta_ms = portTICK_PERIOD_MS*delta_ticks;
      uint32_t bytes = sector_size * sector_count;  // TODO: Use a variable for number of blocks
      float kbytes = bytes/1024.0;
      float kbytes_per_s = (float)bytes/delta_ms;
      printf("\n");
      printf("Read %0.1f KB at %0.3f KB/s\n", kbytes, kbytes_per_s);
    }

    // Restore log level
    g_runtime_log_level = log_level_orig;

dhara_write_read_exit:

  #ifdef DHARA_TESTS_USE_MALLOC  // use static ram if your heap is low
    free(data);
    free(data_out);
  #endif
    printf("\n");
    fflush(stdout);

  } // run_count
}

void dhara_sync_command(int argc, char **argv) {
  struct dhara_map *map = dhara_get_my_map();
  dhara_error_t err;
  int result = dhara_map_sync(map, &err);
  if (0 == result) {
    printf("Map sync success.\n");
  }
  else {
    LOGE(TAG, "dhara_map_sync(): result: %d, err: %d", (int)result, (int)err);
  }
}

void dhara_clear_command(int argc, char **argv) {
  struct dhara_map *map = dhara_get_my_map();
  dhara_map_clear(map);
  printf("Map cleared.\n");
}
