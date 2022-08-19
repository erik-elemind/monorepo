/*
 * fatfs_writer.h
 *
 * Copyright (C) 2020 Igor Institute, Inc.
 *
 * Author(s): Derek Simkowiak <Derek.Simkowiak@igorintitute.com>
 *
 * Description: FreeRTOS Task dedicated to waiting on StreamBuffered slow flash FatFS writes
 */
#ifndef FATFS_WRITER_H
#define FATFS_WRITER_H

#include <stddef.h>
#include "ff.h"

#ifdef __cplusplus
extern "C"{
#endif


// Most of the time a FatFS file write is just a RAM buffer copy, and is very fast.
// But sometimes a write will trigger a 4KB flash sector erase and write, which can take
// (58 ms to 240 ms to erase) plus (164 ms to 410 ms to write) on a mx25r6435f flash chip.
//
// This intermittent write delay of 222 ms (max 650 ms) makes it hard to sample data on
// a regular timer tick, so instead we have a dedicated fatfs_writer_task and
// use a thread-safe FreeRTOS StreamBuffer to write it. This dedicated task can
// then be blocked by the long delay, while the StreamBuffer remains available
// to the calling task. This allows it to continue sampling on a sub-240 ms tick
// into the RAM-based StreamBuffer.
//
// This assumes only one task is calling f_write_nowait() at a time.
//
// It uses a full 4K byte array to buffer up to one sector, so it consumes
// a lot of RAM and should be disabled if not used.

//
// This non-blocking function returns the result of the *previous* internal call
// to f_write(), so that the calling task that repeatedly calls this function
// can eventually get any non-FR_OK error code from f_write() bubbled back up to it,
// so it can stop trying to write after an error.
//
// It is up to the calling task to first use f_open() on the file pointer, and to
// f_sync_wait() before calling f_close() to close the FIL* file handle.
//
// Note the FIL object must not be a temporary stack variable since this task
// expects it to persist until all data has been written.
//
// The first time it's called, it returns FR_OK.
FRESULT f_write_nowait(FIL* fp, const void* buff, UINT btw, UINT* bw);

// Block until both the StreamBuffer and the internal FatFS buffers are flushed.
// This should be called before calling f_close(fp) if using f_write_nowait().
FRESULT f_sync_wait(FIL* fp);

// Launch this dedicated writer task in main.c:
void fatfs_writer_pretask_init(void);
void fatfs_writer_task(void* ignored);

#ifdef __cplusplus
}
#endif

#endif  // FATFS_WRITER_H
