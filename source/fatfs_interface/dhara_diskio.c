/*
 * dhara_diskio.c
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Author(s): Derek Simkowiak <Derek.Simkowiak@igorintitute.com>
 *
 * Description: Implementation of the FatFS user_diskio.c API for a Dhara map.
 */

#include <stdio.h>
#include <string.h>

#include "ff.h"
#include "diskio.h"
#include "dhara_utils.h"

// Set the log level for this file
#define LOG_LEVEL_MODULE  LOG_WARN
#include "loglevels.h"

static const char *TAG = "dhara_diskio";  // Logging prefix for this module

DSTATUS disk_initialize(BYTE drive_number)
{
  LOGD(TAG, "disk_initialize(): drive %d", (int)drive_number);
  
  ; // dhara_pretask_init() is called in main().
  
  return RES_OK;
}

DSTATUS disk_status(BYTE drive_number) { return RES_OK; }

DRESULT disk_read(BYTE drive_number, BYTE *buf, DWORD start, UINT sector_count)
{
  LOGD(TAG, "disk_read(): drive %d, sector %d, count %d", (int)drive_number, (int)start, sector_count);

  int result;
  dhara_error_t err;
  
  struct dhara_map *map = dhara_get_my_map();
  uint16_t sector_size = dhara_map_sector_size_bytes(map);

  // Read all the requested sectors:
  for (int index = 0; index < sector_count; index++) {

    result = dhara_map_read(map, (dhara_sector_t)start, buf, &err);
    if (result != 0) {
      LOGE(TAG, "disk_read(): err: %d, drive %d, sector %d, index %d", (int)err, (int)drive_number, (int)start, index);
      return RES_ERROR;
    }
    
    // Advance the buffer pointer and the sector address:
    start += 1;
    buf += sector_size;
  }

  return RES_OK;
}

DRESULT disk_write(BYTE drive_number, const BYTE *buf, DWORD start, UINT sector_count)
{
  LOGV(TAG, "disk_write(): drive %d, sector %d, count %d", (int)drive_number, (int)start, sector_count);

  int result;
  dhara_error_t err;
  
  struct dhara_map *map = dhara_get_my_map();
  uint16_t sector_size = dhara_map_sector_size_bytes(map);

  // Write all the requested sectors:
  for (int index = 0; index < sector_count; index++) {

    result = dhara_map_write(map, (dhara_sector_t)start, (uint8_t *)buf, &err);
    if (result != 0) {
      LOGE(TAG, "disk_write(): err: %d, drive %d, sector %d, index %d", (int)err, (int)drive_number, (int)start, index);
      return RES_ERROR;
    }
    
    // Advance the buffer pointer and the sector address:
    start += 1;
    buf += sector_size;
  }

#if 0
  result = dhara_map_sync(map, &err);
  if (result != 0) {
    LOGE(TAG, "disk_ioctl(): dhara_map_sync() returned %d, err: %d", result, (int)err);
    result = RES_ERROR;
  }
#endif

  return RES_OK;
}

DRESULT disk_ioctl(BYTE drive_number, BYTE cmd, void *buf)
{
  DRESULT result;

  LOGV(TAG, "disk_ioctl(): drive %d, cmd %d, buf %p", (int)drive_number, (int)cmd, buf);

  struct dhara_map *map = dhara_get_my_map();
  dhara_sector_t sector_count = dhara_map_capacity(map);
  uint16_t sector_size = dhara_map_sector_size_bytes(map);

  int status;
  dhara_error_t err;

  switch (cmd) {
    case CTRL_SYNC:
      result = RES_OK;

      status = dhara_map_sync(map, &err);
      if (status != 0) {
        LOGE(TAG, "disk_ioctl(): dhara_map_sync() returned %d, err: %d", status, (int)err);
        result = RES_ERROR;
      }
      break;

    case GET_BLOCK_SIZE:
      result = RES_PARERR;
      break;

    case GET_SECTOR_SIZE:
      *(WORD *)buf = sector_size;
      result = RES_OK;
      break;

    case GET_SECTOR_COUNT:
    {
      // FatFS requires at least 162 sectors. This is in computed in ff.c line 5593.
      // 162 * 512 = 82944
      //
      // The capacity of the FTL depends on the number of bad blocks
      // encountered and may shrink over the lifetime of the chip.
      //
      // GET_SECTOR_COUNT is only invoked at the point of filesystem
      // creation, and fixes the number of sectors that FATFS expects to
      // have available. We fix it at a slightly discounted value here
      // to give ourselves a margin for shrinkage over the chip's life.
      *(DWORD *)buf = (DWORD)(sector_count - (sector_count >> 4));

      result = RES_OK;
      break;
    }

    default:
      result = RES_ERROR;
      break;
  }

  return (result);
}
