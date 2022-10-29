/*
 * fatfs_writer.h
 *
 * Copyright (C) 2020 Igor Institute, Inc.
 *
 * Author(s): Derek Simkowiak <Derek.Simkowiak@igorintitute.com>
 *
 * Description: FreeRTOS Task dedicated to waiting on StreamBuffered slow flash FatFS writes
 */

#include "fatfs_writer.h"
#include "fatfs_utils.h"
#include "dhara_utils.h"

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"

#include "config.h"
#include "ffconf.h"
#include "nand.h"
#include "loglevels.h"

// Check sizes. FatFS must be configured with a sector size greater than
// the NAND eraseable size
static_assert(FF_MIN_SS >= NAND_PAGE_SIZE, "FatFS must be configured with a sector size greater than the NAND eraseable size");
static_assert(FF_MAX_SS >= NAND_PAGE_SIZE, "FatFS must be configured with a sector size greater than the NAND eraseable size");

static const char* TAG = "fatfs_writer";  // Logging prefix for this module

// One flash page is 4K.
#define FATFS_NOWAIT_STREAM_BUFFER_SIZE (NAND_PAGE_SIZE)
#define FATFS_FSYNC_FLUSH_TIMEOUT_MS 1000
#define FATFS_FSYNC_FLUSH_DELAY_MS 10

#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
static uint8_t g_stream_buffer_array[FATFS_NOWAIT_STREAM_BUFFER_SIZE];
static StaticStreamBuffer_t g_stream_buffer_struct;
static StreamBufferHandle_t g_stream_buffer_handle;

static FRESULT last_result = FR_OK;  // Returned on the next call

static FIL* g_current_file_pointer;
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))

static FRESULT f_delayed_sync(FIL* fp, UINT bw)
{
  static uint32_t g_bytes_written_since_sync = 0;
  FRESULT result = FR_OK;

  // Since we likely have the file open for writes for a long period,
  // try to sync to the file system every 2*PAGE SIZE WRITES.
  // Docs say:
  //   This is suitable for the applications that open files for a long time in write mode, such as data logger. 
  //   Performing f_sync function in certain interval can minimize the risk of data loss due to a sudden blackout, wrong media removal or unrecoverable disk error.
  //   http://elm-chan.org/fsw/ff/doc/sync.html
  g_bytes_written_since_sync += bw;
  if (g_bytes_written_since_sync > (NAND_PAGE_SIZE*2)) {
    // Note that we do not call f_sync_wait() here because it waits for the 
    // writer task to completely drain the streaming buffer.
    // This function is being called from the writer task, so the buffer
    // will not drain. Instead, just sync whatever bytes have been written.
    result = f_sync(fp);
    g_bytes_written_since_sync = 0;
  }

  return result;
}

FRESULT f_write_nowait(FIL* fp, const void* buff, UINT btw, UINT* bw)
{
#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  // Only one task may write at a time, so this g_current_file_pointer
  // should (normally) remain consistent during a given session of writes.
  // We re-assign it to g_current_file_pointer every time just to keep the API
  // simple and mirror the f_write() args.

  // TODO: check the assumption above that the file pointer is not changed 
  //       while we are still writing the previous buffer.
  //       Alternatively: convert to a message buf + circular buf to remove 
  //       the need for this assumption.
  g_current_file_pointer = fp;

  // xStreamBufferSend() returns the the number of bytes written (*bw output).
  // FatFS's f_write() only returns less than bytes_to_write (btw) if the disk
  // is full; here it is based on the stream buffer, so the meaning of
  // (less than btw) is changed. It now means the StreamBuffer is full.
  //
  *bw = (UINT)xStreamBufferSend(g_stream_buffer_handle, buff, btw, portMAX_DELAY);

  // We return the result of the last call to f_write(), so that the writing
  // task has a chance to see than something when wrong during its streaming.
  return last_result;
#else // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  // Task disabled, call write directly.
  FRESULT result = f_write(fp, buff, btw, bw);
  f_delayed_sync(fp, *bw);
  return result;
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
}

FRESULT f_sync_wait(FIL* fp)
{
#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  uint32_t time0_ms = (uint32_t)xTaskGetTickCount();
  uint32_t delta_t = 0;

  BaseType_t buffer_is_empty = xStreamBufferIsEmpty(g_stream_buffer_handle);

  LOGV(TAG, "f_sync_wait(): Flushing StreamBuffer");
  while (buffer_is_empty != pdTRUE && delta_t < FATFS_FSYNC_FLUSH_TIMEOUT_MS) {
    vTaskDelay(10 / portTICK_PERIOD_MS);

    buffer_is_empty = xStreamBufferIsEmpty(g_stream_buffer_handle);
    delta_t = (uint32_t)xTaskGetTickCount() - time0_ms;
  }

  if (buffer_is_empty != pdTRUE) {
    LOGE(TAG, "f_sync_wait(): Could not flush StreamBuffer in %u ms", FATFS_FSYNC_FLUSH_TIMEOUT_MS);
    return FR_DISK_ERR;
  }

  LOGV(TAG, "f_sync_wait(): Flushing FatFS internal buffer to disk");
  return f_sync(fp);  // f_sync() is a blocking call and will sync FatFS buffers to disk
#else // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  LOGV(TAG, "f_sync_wait(): Flushing FatFS internal buffer to disk");
  return f_sync(fp);  // f_sync() is a blocking call and will sync FatFS buffers to disk
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
}

void fatfs_writer_pretask_init(void)
{
#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  static const size_t FATFS_STREAM_TRIGGER_LEVEL_BYTES = 1;

  // Any pre-scheduler init goes here.
  g_stream_buffer_handle = xStreamBufferCreateStatic(
      sizeof(g_stream_buffer_array),
      FATFS_STREAM_TRIGGER_LEVEL_BYTES,
      g_stream_buffer_array,
      &(g_stream_buffer_struct) );
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
}

#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
// Execute any deferred bad-block relocations
static void try_relocate(void)
{
  struct dhara_map *map = dhara_get_my_map();
  dhara_error_t err;
  int status;

  if (!fatfs_lock()) {
    LOGE(TAG, "fatfs_lock() timed out");
    return;
  }

  status = dhara_map_preemptive_recover(map, &err);

  if (status < 0) {
    LOGE(TAG, "dhara_map_preemptive_recover() error: %d", err);
  } else if (status) {
    LOGW(TAG, "dhara_map_preemptive_recover() relocated a failing block");

    // This may not be the last block -- try again on the next writeback
    // cycle.
    dhara_recovery_hint_set();
  }

  fatfs_unlock();
}
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))

void fatfs_writer_task(void* ignored)
{
#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
  size_t byte_count;
  UINT bytes_written;
  static uint8_t buffer[FF_MIN_SS];  // f_write() up to one FatFS sector at a time

  while (1) {
    // Wait for another task to write to us:
    byte_count = xStreamBufferReceive(g_stream_buffer_handle, buffer, sizeof(buffer), portMAX_DELAY);

    // Got some bytes. Write them, with a potential blocking delay:
    last_result = f_write(g_current_file_pointer, buffer, byte_count, &bytes_written);

    if (FR_OK != last_result || bytes_written < byte_count) {
      LOGW(TAG, "f_write() error %u", last_result);
    }

    // Sync the file system periodically
    f_delayed_sync(g_current_file_pointer, byte_count);

    if (dhara_recovery_hint_take()) {
      try_relocate();
    }
  }  // while(1)
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
}
