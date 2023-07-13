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
#ifndef FATFS_UTILS_H
#define FATFS_UTILS_H

#include "ff.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// FatFs f_mkfs() formatting options:
//#define FATFS_FORMAT_WORKING_BUFFER_SIZE (2048)
#define FATFS_FORMAT_WORKING_BUFFER_SIZE (512 * 4) // Must be big enough for an AU
//#define FATFS_ALLOCATION_UNIT_SIZE (512 * 8)  // Use 4 KB AU
#define FATFS_ALLOCATION_UNIT_SIZE (512 * 4)  // Use 2 KB AU

FRESULT format_drive(void);
FRESULT mount_fatfs_drive_and_format_if_needed(void);
FRESULT rm_star_dot_star(char *path);
FRESULT ymodem_star_dot_star(char *path);

// Return the first unused filename of format logN.csv, with N an integer.
// Simple brute force: if log1.csv and log3.csv exist, this will return log2.csv.
FRESULT get_next_filename(char *buffer, size_t buffer_size);

// Get number of bytes free in the filesystem.
// Either parameter is optional. Set to NULL if not needed.
// Returns FR_OK on success.
FRESULT f_getfreebytes(DWORD* bytes_free, DWORD* bytes_total);

// Returns 1 if the file object refers to an open file.
bool f_is_open(FIL* f);

// Lock/unlock the filesytem. fatfs_lock returns non-zero if the lock
// was acquired (0 in the case of a timeout).
int fatfs_lock(void);
void fatfs_unlock(void);

#ifdef __cplusplus
}
#endif

#endif  // FATFS_UTILS_H
