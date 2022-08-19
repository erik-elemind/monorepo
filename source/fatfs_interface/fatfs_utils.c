/*
 * fatfs_utils.c
 *
 * Copyright (C) 2019-2020 Igor Institute, Inc.
 *
 * Author(s):
 *      Derek Simkowiak <Derek.Simkowiak@igorintitute.com>
 *
 * Description: Utility functions using the FatFS API to mount drives and manage files.
 */

#include "fatfs_utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils.h"
#include "loglevels.h"
#include "ymodem.h"
#include "rtc.h"
#include "nand_commands.h"

// FatFS time structure, returned by get_fattime()
// http://elm-chan.org/fsw/ff/doc/fattime.html
typedef union {
  struct {
    DWORD sec_div2          : 5; // bit4:0   Second / 2 (0..29, e.g. 25 for 50)
    DWORD minute            : 6; // bit10:5  Minute (0..59)
    DWORD hour              : 5; // bit15:11 Hour (0..23)
    DWORD day               : 5; // bit20:16 Day of the month (1..31)
    DWORD month             : 4; // bit24:21 Month (1..12)
    DWORD year_since_1980   : 7; // bit31:25 Year origin from the 1980 (0..127, e.g. 37 for 2017)
  };
  DWORD dword;
} fattime_t;
static_assert(sizeof(fattime_t) == sizeof(DWORD));

static const char* TAG = "fatfs_utils";  // Logging prefix for this module
static const char my_drive_path[] = "0:";  // Use the default disk.

FRESULT format_drive(void)
{
  FRESULT result;
  // WARN: This greatly inflates the calling task's stack requirements.
  //uint8_t buffer[FATFS_FORMAT_WORKING_BUFFER_SIZE];  // Needed for mkfs(); not static.

  uint8_t *buffer = malloc(FATFS_FORMAT_WORKING_BUFFER_SIZE);  // Needed for mkfs(); not static.

  static const MKFS_PARM mkfs_param = {
    .fmt = (FM_SFD | FM_ANY),
    .au_size = FATFS_ALLOCATION_UNIT_SIZE,
  };
  result = f_mkfs(my_drive_path, &mkfs_param, buffer,
                  FATFS_FORMAT_WORKING_BUFFER_SIZE);
  free(buffer);
  if (result == FR_OK) {
    LOGI(TAG, "...f_mkfs() returned FR_OK. You may now use the FATfs filesystem.");
  } else {
    LOGE(TAG, "...f_mkfs() returned %d", (int)result);
  }

  return result;
}

// NOTE: The FatFs library keeps a pointer to this struct for future
// file operations, so this must be static or global so it stays around.
//
/* File system object for User logical drive */
static FATFS my_drive;

FRESULT mount_fatfs_drive_and_format_if_needed(void)
{
  FRESULT result;

  BYTE force_immediate_mount = 1;

  result = f_mount(&my_drive, (TCHAR const*)my_drive_path, force_immediate_mount);
  if (result == FR_NO_FILESYSTEM) {
    // It's a new / empty disk. Format it.
    LOGI(TAG, "f_mount() returned FR_NO_FILESYSTEM, calling f_mkfs(). Please standby...");


#if _MULTI_PARTITION
    // This dynamically sized global array from upstream fatfs.c must be defined:
    // WARN: The STM32CubeMX export does not assign a size, causing a gcc warning.
    // stm32cubemx/nucleo-l476rg/FATFS/App/fatfs.c:30:11: warning: array 'VolToPart' assumed to have one element
    //
    // See also:
    // http://elm-chan.org/fsw/ff/doc/fdisk.html
    PARTITION fatfs_sda1 = {0, 1};
    VolToPart[0] = fatfs_sda1;    /* "0:" ==> Physical drive 0, 1st partition */

    DWORD plist[] = {100, 0, 0, 0};  /* Divide the drive into one partition (100%) */

    result = f_fdisk(0, plist, buffer);                    /* Divide physical drive 0 */
    if (result != FR_OK) {
      LOGE(TAG, "...f_fdisk() returned %d", (int)result);
      return result;
    }
#endif

    // erase the nand blocks
    char* argv0 = "fatfs_utils";
    char** argv = &argv0;
    nand_safe_wipe_all_blocks_command(1, argv);

    result = format_drive();
    if (result != FR_OK) {
      return result;
    }

    // Now that it's formatted, try mounting it again:
    result = f_mount(&my_drive, (TCHAR const*)my_drive_path, force_immediate_mount);
    if (result != FR_OK) {
      LOGE(TAG, "f_mount() failed with %d after calling f_mkfs(). Giving up.", (int)result);
      return result;
    }
  }
  else if (result != FR_OK) {
    LOGE(TAG, "f_mount() returned %d", (int)result);
    return result;
  }
  if (result != FR_OK) {
     LOGE(TAG, "f_mount() returned %d", (int)result);
     return result;
   }

  LOGD(TAG, "f_mount() returned FR_OK");

  return result;
}

/* path: Start node to be scanned (***also used as work area***) */
FRESULT rm_star_dot_star(char* path)
{
  FRESULT result;
  DIR dir;
  FILINFO file_info;
  char full_path_buffer[256];

  result = f_opendir(&dir, path); /* Open the directory */
  if (result == FR_OK) {
    for (;;) {
      result = f_readdir(&dir, &file_info); /* Read a directory item */
      if (result != FR_OK || file_info.fname[0] == 0)
        break; /* Break on error or end of dir */

      sprintf(full_path_buffer, "%s/%s\n", path, file_info.fname);
      result = f_unlink(full_path_buffer);
      if (result != FR_OK) {
        LOGE(TAG, "f_unlink() returned %d for path %s", (int)result, full_path_buffer);
        break;
      }
    }
    f_closedir(&dir);
  }

  return result;
}

FRESULT ymodem_star_dot_star(
    char* path /* Start node to be scanned (***also used as work area***) */
)
{
  FRESULT fatfs_result;  // enum (unsigned)
  int result_int;        // signed, for ymodem_send_file()
  DIR dir;
  FILINFO file_info;

  fatfs_result = f_opendir(&dir, path); /* Open the directory */
  if (fatfs_result == FR_OK) {
    for (;;) {
      fatfs_result = f_readdir(&dir, &file_info); /* Read a directory item */
      if (fatfs_result != FR_OK || file_info.fname[0] == 0)
        break; /* Break on error or end of dir */

      result_int = ymodem_send_file(&uart_interface, file_info.fname);
      if (result_int < 0) {
        fatfs_result = FR_INT_ERR;
        LOGI(TAG, "YMODEM Transfer aborted");
        break;
      }
    }
    f_closedir(&dir);
  }

  ymodem_end_session();

  return fatfs_result;
}


FRESULT get_next_filename(char* buffer, size_t buffer_size)
{
  FRESULT result;
  DIR dir;
  static FILINFO file_info;
  int index = 0;

  // Support up to a billion file names, with names in the form:
  if (buffer_size < 20) {
    LOGE(TAG, "Insufficient buffer_size for get_next_filename()");
    return FR_INT_ERR;
  }

//  result = f_opendir(&dir, "0:/"); /* Open the root directory */
  result = f_opendir(&dir, "/"); /* Open the root directory */

  if (result != FR_OK) {
    LOGE(TAG, "f_opendir() returned %d", (int)result);
    return result;
  }

  // Run this loop until we find an unused file name:
  while (TRUE) {
    index++;

    // Create the next candidate filename:
    sprintf(buffer, "log%d.csv", index);

    // See if this candidate exists in the directory.
    // This loop never comes out; it just returns with success or failure code
    while (TRUE) {
      result = f_readdir(&dir, &file_info); /* Read a directory item */
      if (result != FR_OK) {
        LOGE(TAG, "f_readdir() returned %d", (int)result);
        f_closedir(&dir);
        return result;
      }

      if (file_info.fname[0] == 0) {
        /* End of dir reached, we found our candidate; it's in the output buffer. */
        LOGV(TAG, "Found unused filename %s", buffer);
        f_closedir(&dir);
        return FR_OK;
      }

      // Do a case-insensitive file name check:
      if (strncasecmp(file_info.fname, buffer, buffer_size) == 0) {
        // The candidate filename already exists. Try the next candidate.
        LOGV(TAG, "File %s already exists, trying the next one", file_info.fname);
        break;
      }
      else {
        // This dir entry is not our candidate, so check the next dir entry.
        continue;
      }
    }
  }
}

// Return a fatfs time structure based on RTC
DWORD get_fattime(void) {
  fattime_t ft = {0};

  time_t t = rtc_get();
  struct tm* ptm = gmtime(&t);

  if (ptm) {
    // divide by 2 to get the fattime seconds field
    ft.sec_div2         = ptm->tm_sec >> 1;
    ft.minute           = ptm->tm_min;
    ft.hour             = ptm->tm_hour;
    ft.day              = ptm->tm_mday;
    // struct tm month field is 0-11, so add 1
    ft.month            = ptm->tm_mon + 1;
    // struct tm year field is years since 1900
    // so subtract 80 to get years since 1980
    ft.year_since_1980  = ptm->tm_year - 80;
  }

  return ft.dword;
}

bool f_is_open(FIL* f)
{
  return (bool)f->obj.fs;
}

FRESULT f_getfreebytes(DWORD* bytes_free, DWORD* bytes_total)
{
  // Get free space in filesystem.
  // Based on example from here: 
  // http://elm-chan.org/fsw/ff/doc/getfree.html
  DWORD fre_clust, fre_sect, tot_sect, fre_bytes, tot_bytes;
  FATFS* fs;
  FRESULT result;

  result = f_getfree("0:", &fre_clust, &fs);
  if (result) {
    LOGE(TAG, "f_getfree() returned %u\n", result);
    return result;
  }
  /* Get total sectors and free sectors */
  tot_sect = (fs->n_fatent - 2) * fs->csize;
  fre_sect = fre_clust * fs->csize;
  /* Convert to bytes */
  tot_bytes = tot_sect * FF_MIN_SS;
  fre_bytes = fre_sect * FF_MIN_SS;

  if (bytes_free) {
    *bytes_free = fre_bytes;
  }
  if (bytes_total) {
    *bytes_total = tot_bytes;
  }

  return FR_OK;
}

// This pair of functions act on the same synchronization object which
// is acquired by filesystem operations in fatfs. The purpose of
// acquiring the lock explicitly outside of fatfs is to ensure that no
// filesystem operations are occuring concurrently.

int fatfs_lock(void)
{
  return ff_req_grant(my_drive.sobj);
}

void fatfs_unlock(void)
{
  ff_rel_grant(my_drive.sobj);
}
