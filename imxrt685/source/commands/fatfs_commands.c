#include <fatfs_commands.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fatfs_utils.h"

#include "FreeRTOS.h"
#include "task.h"
//#include "ymodem.h"

// Set the log level for this file
#define LOG_LEVEL_MODULE  LOG_WARN
#include "loglevels.h"

const char *TAG = "fatfs_commands";  // Logging prefix for this module

#define FILE_WORKING_BUFFER_SIZE 128

void format_disk_command(int argc, char **argv)
{
  FRESULT result;
  char my_drive_path[] = "0:"; /* User logical drive path */

  uint8_t buffer[FATFS_FORMAT_WORKING_BUFFER_SIZE];  // Needed for mkfs(); not static.

  static const MKFS_PARM mkfs_param = {
    .fmt = (FM_SFD | FM_ANY),
    .au_size = FATFS_ALLOCATION_UNIT_SIZE,
  };
  result = f_mkfs(my_drive_path, &mkfs_param, buffer,
                  FATFS_FORMAT_WORKING_BUFFER_SIZE);
  if (result != FR_OK) {
    LOGE(TAG, "f_mkfs() returned %d", (int)result);
    return;
  }
  LOGI(TAG, "f_mkfs() returned FR_OK");

  // If experimenting and this is not called elsewhere, mount it:
  // mount_fatfs_drive_and_format_if_needed();
}

void filetest_command(int argc, char **argv)
{
  FIL my_file; /* File object */
  FRESULT result;

  // These var names are based on the STM example code.

  uint32_t wbytes; /* File write counts */
  static const uint8_t wtext[] =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt "
      "ut labore et dolore magna aliqua.";

  unsigned int rbytes; /* File read counts */

#ifdef FATFS_COMMAND_STATIC
  uint8_t rtext[FILE_WORKING_BUFFER_SIZE];
  char filename[FILE_WORKING_BUFFER_SIZE];
#else
  uint8_t* rtext = malloc(FILE_WORKING_BUFFER_SIZE);
  char* filename = malloc(FILE_WORKING_BUFFER_SIZE);
  if (rtext == NULL || filename == NULL) {
    LOGE(TAG, "could not allocate working buffers\n");
    goto filetest_command_exit;
  }
#endif

  memset(rtext, 0x0, FILE_WORKING_BUFFER_SIZE);
  memset(filename, 0x0, FILE_WORKING_BUFFER_SIZE);

  result = get_next_filename(filename, FILE_WORKING_BUFFER_SIZE);
  if (result != FR_OK) {
    LOGE(TAG, "get_next_filename() returned %d", (int)result);
    goto filetest_command_exit;
  }

  result = f_open(&my_file, filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (result != FR_OK) {
    LOGE(TAG, "f_open() for write returned %d", (int)result);
    goto filetest_command_exit;
  }
  LOGI(TAG, "f_open() returned FR_OK");

  result = f_write(&my_file, wtext, sizeof(wtext), (void *)&wbytes);
  if (result != FR_OK) {
    LOGE(TAG, "f_write() returned %d", (int)result);
    goto filetest_command_exit;
  }
  LOGI(TAG, "f_write() returned FR_OK, wrote %d bytes", (int)wbytes);

  result = f_close(&my_file);
  if (result != FR_OK) {
    LOGE(TAG, "f_close() returned %d", (int)result);
    goto filetest_command_exit;
  }
  LOGI(TAG, "f_close() returned FR_OK");

  result = f_open(&my_file, filename, FA_READ);
  if (result != FR_OK) {
    LOGE(TAG, "f_open() for read returned %d", (int)result);
    goto filetest_command_exit;
  }
  LOGI(TAG, "f_open() returned FR_OK");

  result = f_read(&my_file, rtext, FILE_WORKING_BUFFER_SIZE, &rbytes);
  if (result != FR_OK) {
    LOGE(TAG, "f_read() returned %d", (int)result);
    goto filetest_command_exit;
  }
  LOGI(TAG, "f_read() returned FR_OK, read %d bytes", (int)rbytes);

  result = f_close(&my_file);
  if (result != FR_OK) {
    LOGE(TAG, "f_close() returned %d", (int)result);
    goto filetest_command_exit;
  }
  LOGI(TAG, "f_close() returned FR_OK");

//  printf("Read back %s: \n\n%s", filename, rtext);
  printf("Read back %s: \n\n", filename);
  printf("%s\n", rtext);

  filetest_command_exit:
#ifdef FATFS_COMMAND_STATIC
  // buffers statically allocated, nothing to do
#else
  free(rtext);
  free(filename);
#endif
}

// Example function pasted from the FatFs docs:
static FRESULT scan_files(
    char *path /* Start node to be scanned (***also used as work area***) */
)
{
  FRESULT res;
  DIR dir;
  UINT i;
  FILINFO fno;

  res = f_opendir(&dir, path); /* Open the directory */
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno); /* Read a directory item */
      if (res != FR_OK || fno.fname[0] == 0)
        break;                    /* Break on error or end of dir */
      if (fno.fattrib & AM_DIR) { /* It is a directory */
        i = strlen(path);
        sprintf(&path[i], "%s", fno.fname);
        res = scan_files(path); /* Enter the directory */
        if (res != FR_OK)
          break;
        path[i] = 0;
      }
      else { /* It is a file. */
        printf("%-13s %7d\n", fno.fname, (int)fno.fsize);
      }
    }
    f_closedir(&dir);
  }

  return res;
}

void ls_command(int argc, char **argv)
{
  char* buffer = malloc(256);
  if (buffer == NULL) {
    return;
  }
  memset(buffer, 0x0, 256);
  scan_files(buffer);
}

void rm_command(int argc, char **argv)
{
  FRESULT result;
  char buffer[256];

  if (argc < 2) {
    printf("missing filename argument, or * for all, e.g.: rm run1.csv");
    return;
  }

  uint32_t len = strlen(argv[1]);
  if (len >= sizeof(buffer)) {
    LOGE(TAG, "rm: command too long");
    return;
  }

  if (len == 1 && argv[1][0] == '*') {
    LOGV(TAG, "Found wildcard '*', deleting all files");

    memset(buffer, 0x0, 256);
    result = rm_star_dot_star(buffer);
    if (result != FR_OK) {
      LOGE(TAG, "rm: Could not erase *");
    }
  }
  else if (len >= 2 && (argv[1][len-1] == '*' && argv[1][len-2] == '/')) {
     LOGV(TAG, "Found wildcard '*', deleting all files from dir %s", argv[1]);

    // Copy path without the trailing "/*"
    memcpy(buffer, argv[1], len-2);
    buffer[len-2] = '\0';
    result = rm_star_dot_star(buffer);
    if (result != FR_OK) {
      LOGE(TAG, "rm: Could not erase %s", buffer);
    }
  }
  else {
    LOGV(TAG, "Deleting file %s", argv[1]);
    result = f_unlink(argv[1]);
    if (result != FR_OK) {
      LOGE(TAG, "f_unlink() returned %d", (int)result);
      return;
    }
    LOGV(TAG, "f_unlink() returned FR_OK for file %s", argv[1]);
  }
}

void printfile(char *filename, int max_bytes)
{
  FIL my_file; /* File object */
  FRESULT result;

  unsigned int total_bytes_read, bytes_read, bytes_desired;
  uint8_t chunk_buffer[FILE_WORKING_BUFFER_SIZE];

  result = f_open(&my_file, filename, FA_READ);
  if (result != FR_OK) {
    LOGE(TAG, "f_open() for read of %s returned %d", filename, (int)result);
    return;
  }

  total_bytes_read = 0;
  while (total_bytes_read < max_bytes) {
    bytes_desired = MIN(sizeof(chunk_buffer), max_bytes - total_bytes_read);

    result = f_read(&my_file, chunk_buffer, bytes_desired, &bytes_read);
    if (result != FR_OK) {
      LOGE(TAG, "f_read() returned %d", (int)result);
      return;
    }

    // %.*s prints N bytes of the string, avoiding the need for a terminating NULL
    printf("%.*s", bytes_read, chunk_buffer);

    // delay a little bit to let the bytes get out over the wires.
    vTaskDelay(10);

    if (bytes_read < bytes_desired) {
      // End-of-file reached.
      break;
    }

    total_bytes_read += bytes_read;
  }

  result = f_close(&my_file);
  if (result != FR_OK) {
    LOGE(TAG, "f_close() returned %d", (int)result);
    return;
  }

  return;
}

void cat_command(int argc, char **argv)
{
  if (argc < 2) {
    printf("Usage: cat [filename], e.g.: cat log1.csv");
    return;
  }

  printfile(argv[1], INT32_MAX);
}

void head_command(int argc, char **argv)
{
  int max_bytes;

  if (argc < 2) {
    printf(
        "Usage: head [filename], e.g.: head log1.csv\n"
        "Provide an optional integer 3rd argument for \n"
        "number of bytes, e.g.: head log2.csv 2048");
    return;
  }

  max_bytes = 300;
  if (argc == 3) {
    max_bytes = atoi(argv[2]);
  }

  printfile(argv[1], max_bytes);
}
#if 0
void ymodem_command(int argc, char **argv)
{
  FIL my_file; /* File object */
  FRESULT result;
  unsigned int filesize;
  char buffer[256];
  uint8_t flush_byte;

  if (argc < 2) {
    printf(
        "Usage: ymodem [filename], e.g.: ymodem log1.csv\n"
        "Use asterisk (*) to send all files, e.g.: ymodem *");
    return;
  }

  if (argv[1][0] == '*') {
    printf(
        "Please start YMODEM receiving in your terminal program.\n"
        "Sending YMODEM batch send (all files) in 5 seconds...\n\n");
    // Clear the UART input buffer before sending file(s):
    vTaskDelay((5000) / portTICK_PERIOD_MS);
    // while (getchar() >= 0);  // This is now blocking, cannot be used to flush.
    // Use the task_io blocking function with a short timeout:
    while (hal_uart_receive_wait(&SHELL_UART_HANDLE, &flush_byte, sizeof(flush_byte), 1) == HAL_OK)
      ;

    memset(buffer, 0x0, 256);
    ymodem_star_dot_star(buffer);
  }
  else {
    printf(
        "Please start YMODEM receiving in your terminal program.\n"
        "Starting YMODEM send of file %s in 5 seconds...\n\n",
        argv[1]);
    // Clear the UART input buffer before sending file(s):
    vTaskDelay((5000) / portTICK_PERIOD_MS);

    // while (getchar() >= 0);  // This is now blocking, cannot be used to flush.
    // Use the task_io blocking function with a short timeout:
    while (hal_uart_receive_wait(&SHELL_UART_HANDLE, &flush_byte, sizeof(flush_byte), 1) == HAL_OK)
      ;

    // Get the filesize, needed by ymodem_send_file():
    result = f_open(&my_file, argv[1], FA_READ);
    if (result != FR_OK) {
      LOGE(TAG, "f_open() for read of %s returned %d", argv[1], (int)result);
      return;
    }

    filesize = f_size(&my_file);

    result = f_close(&my_file);
    if (result != FR_OK) {
      LOGE(TAG, "f_close() returned %d", (int)result);
      return;
    }

    ymodem_send_file(argv[1], filesize);
    ymodem_end_session();
  }
}
#endif
