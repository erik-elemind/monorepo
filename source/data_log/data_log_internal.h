/*
 * data_log_internal.h
 *
 * Copyright (C) 2022 Elemind Technologies, Inc.
 *
 * Created: Apr, 2022
 * Author:  Bradey Honsinger
 *
 * Description: Data log module internal definitions.
 */

#ifndef DATA_LOG_INTERNAL_H
#define DATA_LOG_INTERNAL_H

#include "data_log_buffer.h" // needed for DLBuffer
#include "FreeRTOS.h" // needed for TickType_t

#define MAX_PATH_LENGTH 128
#define DATA_LOG_DIR_PATH "/datalogs"

//
// Task events:
//
typedef enum
{
  DATA_LOG_EVENT_ENTER_STATE,  // (used for state transitions)
  DATA_LOG_EVENT_OPEN,
  DATA_LOG_EVENT_CLOSE,
  DATA_LOG_SET_TIME,
  DATA_LOG_EVENT_READY_TO_SEND,
  DATA_LOG_EEG_DATA,
  DATA_LOG_INST_DATA,
  DATA_LOG_EVENT_COMPRESS,  // TODO: _EEG_COMPRESS_START instead?
  DATA_LOG_EVENT_COMPRESS_ABORT,
  DATA_LOG_EVENT_COMPRESS_DATA,
  DATA_LOG_EVENT_COMPRESS_NEXT,
} data_log_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  data_log_event_type_t type;
  uint8_t *msg;
  size_t msg_size;
} data_log_event_t;

/*
 * Specify which packet types should be logged
 */

#ifndef ENABLE_DATA_LOG_STREAM_ACCEL
#define ENABLE_DATA_LOG_STREAM_ACCEL (0U) // (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_ACCEL_TEMP
#define ENABLE_DATA_LOG_STREAM_ACCEL_TEMP (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_COMMAND
#define ENABLE_DATA_LOG_STREAM_COMMAND (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_ALS
#define ENABLE_DATA_LOG_STREAM_ALS (0U) // (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_MIC
#define ENABLE_DATA_LOG_STREAM_MIC (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_TEMP
#define ENABLE_DATA_LOG_STREAM_TEMP (0U) // (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_PULSE
#define ENABLE_DATA_LOG_STREAM_PULSE (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_ECHT_CHANNEL
#define ENABLE_DATA_LOG_STREAM_ECHT_CHANNEL (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_STIM_SWITCH
#define ENABLE_DATA_LOG_STREAM_STIM_SWITCH (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_EEG
#define ENABLE_DATA_LOG_STREAM_EEG (1U) // (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_INST
#define ENABLE_DATA_LOG_STREAM_INST (0U) // (0U)
#endif

#ifndef ENABLE_DATA_LOG_STREAM_STIM
#define ENABLE_DATA_LOG_STREAM_STIM (0U)
#endif

/*
 * Specify that log files should be saved to eMMC memory
 */

#ifndef ENABLE_DATA_LOG_SAVE_TO_LOFFILE
#define ENABLE_DATA_LOG_SAVE_TO_LOFFILE 1U
#endif

/*
 * Data log encoding
 */

#ifndef CONFIG_DATALOG_USE_COBSR_RLE0
#define CONFIG_DATALOG_USE_COBSR_RLE0 (1U)
#endif

#ifndef CONFIG_DATALOG_USE_TIME_STRING
#define CONFIG_DATALOG_USE_TIME_STRING (1U)
#endif

#ifndef ENABLE_OFFLINE_EEG_COMPRESSION
#define ENABLE_OFFLINE_EEG_COMPRESSION (0U)
#endif

// Memory management functions defined in data_log.cpp
void* dl_malloc(size_t size);
void* dl_malloc_if_file_ready(size_t size);
void dl_free(void* ptr);
void send_data(DLBuffer *dlbuf, TickType_t xTicksToWait);


void add_to_buffer(uint8_t *buf, size_t buf_size, size_t &offset, void* data, size_t data_size);
void add_to_buffer(uint8_t *buf, size_t buf_size, size_t &offset, uint8_t data);

void send_data(uint8_t *scratch, uint32_t scratch_size, data_log_event_type_t event_type, TickType_t xTicksToWait);
void send_data(uint8_t *scratch, uint32_t scratch_size, TickType_t xTicksToWait);

void data_log_write(const uint8_t *scratch, uint32_t scratch_size);

bool open_eeg_comp_data_log(const char* log_filename_to_compress);

#endif  // DATA_LOG_INTERNAL_H
