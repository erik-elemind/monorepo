/*
 * data_log.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Oct, 2020
 * Author:  David Wang
 */

#include <stdlib.h>
#include <stdbool.h>
#include <algorithm>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "message_buffer.h"

#include "micro_clock.h"
#include "utils.h"
#include "string_util.h"
#include "rtc.h"

#include "ff.h"
#include "fatfs_writer.h"
#include "fatfs_utils.h"

#include "COBS.h"
#include "COBSR_RLE0.h"
#include "data_log.h"
#include "data_log_internal.h"
#include "data_log_eeg_comp.h"
#include "data_log_buffer.h"
#include "data_log_buffer_packed.h"
#include "data_log_parse.h"
#include "data_log_packet.h"

#include "memman.h"
#include "memman_rtos.h"
#include "loglevels.h"
#include "config.h"
#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "heatshrink_encoder.h"
#include "compression.h"

#ifdef __cplusplus
}
#endif

#if (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))

static const char *TAG = "data_log";  // Logging prefix for this module

#define USE_SRAMX_DATA_LOG_BUFFER (0U)

#if (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))
#define DATA_LOG_NUM_MSG 270 //300
#else
#define DATA_LOG_NUM_MSG 800
#endif // (defined(ENABLE_FS_WRITER_TASK) && (ENABLE_FS_WRITER_TASK > 0U))

#define DATA_LOG_EVENT_QUEUE_SIZE DATA_LOG_NUM_MSG

#define DATETIME_STRING_MAX_SIZE 30

#define SRC_SCRATCH_BUFFER_SIZE 256
#define DST_SCRATCH_BUFFER_SIZE 1700 // used to be 2000
#define HSE_SCRATCH_BUFFER_SIZE ( DST_SCRATCH_BUFFER_SIZE + (DST_SCRATCH_BUFFER_SIZE/2) + 4 )

#define MEMORY_MANAGER_BUFFER_SIZE (DATA_LOG_NUM_MSG*125)

#define DATA_LOG_USE_LOCAL_MEMORY_MANAGER (1U)

typedef enum
{
  DATA_LOG_STATE_CLOSED,
  DATA_LOG_STATE_OPENED,
  DATA_LOG_STATE_COMPRESSING,
} data_log_state_t;

//
// Global context data:
//
typedef struct
{
  // state
  data_log_state_t state;

  // log file
  char log_filename_to_compress[MAX_PATH_LENGTH] = {0};
  bool log_compression_successful = false;

  FIL  open_log_file;

  // encoding & compression
  uint8_t dst_scratch[DST_SCRATCH_BUFFER_SIZE];
  uint8_t hse_scratch[HSE_SCRATCH_BUFFER_SIZE];
  heatshrink_encoder hse;

  // logging_enabled flag
  bool file_ready;
  SemaphoreHandle_t file_ready_sem = NULL;
  StaticSemaphore_t file_ready_sem_buf;

#if (defined(CONFIG_DATALOG_USE_TIME_STRING) && (CONFIG_DATALOG_USE_TIME_STRING > 0U))
  char datetime[DATETIME_STRING_MAX_SIZE];
  size_t datetime_size = 0;
#endif
} data_log_context_t;

//
// Event user data:
//

static data_log_context_t g_data_log_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[DATA_LOG_EVENT_QUEUE_SIZE*sizeof(data_log_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(data_log_event_t *event);

// For logging and debug:
static const char *
data_log_state_name(data_log_state_t state)
{
  switch (state) {
    case DATA_LOG_STATE_CLOSED:      return "DATA_LOG_STATE_CLOSED";
    case DATA_LOG_STATE_OPENED:      return "DATA_LOG_STATE_OPENED";
    case DATA_LOG_STATE_COMPRESSING: return "DATA_LOG_STATE_COMPRESSING";
    default:
      break;
  }
  return "DATA_LOG_STATE UNKNOWN";
}

static const char *
data_log_event_type_name(data_log_event_type_t event_type)
{
  switch (event_type) {
    case DATA_LOG_EVENT_ENTER_STATE:     return "DATA_LOG_EVENT_ENTER_STATE";
    case DATA_LOG_EVENT_OPEN:            return "DATA_LOG_EVENT_OPEN";
    case DATA_LOG_EVENT_CLOSE:           return "DATA_LOG_EVENT_CLOSE";
    case DATA_LOG_SET_TIME:              return "DATA_LOG_SET_TIME";
    case DATA_LOG_EVENT_READY_TO_SEND:   return "DATA_LOG_EVENT_READY_TO_SEND";
    case DATA_LOG_EEG_DATA:              return "DATA_LOG_EEG_DATA";
    case DATA_LOG_INST_DATA:             return "DATA_LOG_INST_DATA";
    default:
      break;
  }
  return "DATA_LOG_EVENT UNKNOWN";
}

/*****************************************************************************/
// Logging enabled

static bool get_file_ready(){
  bool enabled = false;
  if( xSemaphoreTake( g_data_log_context.file_ready_sem, portMAX_DELAY ) == pdTRUE ) {
    enabled = g_data_log_context.file_ready;
    xSemaphoreGive( g_data_log_context.file_ready_sem );
  }
  return enabled;
}

static void set_file_ready(bool val){
  if( xSemaphoreTake( g_data_log_context.file_ready_sem, portMAX_DELAY ) == pdTRUE ) {
    g_data_log_context.file_ready = val;
    xSemaphoreGive( g_data_log_context.file_ready_sem );
  }
}

/*****************************************************************************/
// Memory Manager

#if (defined(DATA_LOG_USE_LOCAL_MEMORY_MANAGER) && (DATA_LOG_USE_LOCAL_MEMORY_MANAGER > 0U))
uint8_t g_mm_buf[MEMORY_MANAGER_BUFFER_SIZE];
mm_rtos_t g_mm_rtos;
void* dl_malloc(size_t size){
  return mm_rtos_malloc (&g_mm_rtos, size, portMAX_DELAY);
}
void dl_free(void* ptr){
    mm_rtos_free(&g_mm_rtos, ptr);
}

#else
uint8_t src_scratch[SRC_SCRATCH_BUFFER_SIZE];
void* dl_malloc(size_t size){
  return src_scratch;
}
void dl_free(void* ptr){
  // do nothing
}
#endif

void* dl_malloc_if_file_ready(size_t size){
  return get_file_ready() ? dl_malloc(size) : NULL;
}

/*****************************************************************************/
// Send data helper functions

void send_data(uint8_t *scratch, uint32_t scratch_size, data_log_event_type_t event_type, TickType_t xTicksToWait) {
  if(scratch == NULL){
    LOGE(TAG,"send_data() scratch is NULL");
    return;
  }
  data_log_event_t event = {.type = event_type, .msg = scratch, .msg_size = scratch_size};
  BaseType_t status = xQueueSend(g_event_queue, &event, xTicksToWait); //portMAX_DELAY
  if(status != pdTRUE){
    // enqueue has failed
    LOGE(TAG,"send_data() enqueue has failed");
    dl_free(scratch);
  }
}

void send_data(uint8_t *scratch, uint32_t scratch_size, TickType_t xTicksToWait) {
  send_data(scratch, scratch_size, DATA_LOG_EVENT_READY_TO_SEND, xTicksToWait);
}

void
send_data(DLBuffer *dlbuf, TickType_t xTicksToWait) {
  send_data(dlbuf->buf_, dlbuf->buf_off_, DATA_LOG_EVENT_READY_TO_SEND, xTicksToWait);
}

/*****************************************************************************/
// Local log file manipulation functions


#if (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))

#if 0
static size_t CountObjectUnder(const char *path)
{
  size_t count = 0;
  DIR dir;
  FILINFO finfo;

  FRESULT result = f_opendir(&dir, path);
  if (FR_OK == result) {
    result = f_readdir(&dir, &finfo);
    // FatFS indicates end of directory with a null name
    while (FR_OK == result && 0 != finfo.fname[0]) {
      count++;
      result = f_readdir(&dir, &finfo);
    }

    f_closedir(&dir);
  }
  return count;
}
#endif

static const char* settings_datalog_key = "datalog.uid";

// Returns true if the setting exists
static bool getLogFileUID(char* uid, size_t uid_size){
  return (0 == settings_get_string(settings_datalog_key, uid, uid_size));
}

static bool setLogFileUID(char* uid){
  return (0 == settings_set_string(settings_datalog_key, uid));
}

static void close_data_log(){
  if(f_is_open(&g_data_log_context.open_log_file)){
    f_sync_wait(&g_data_log_context.open_log_file);
    f_close(&g_data_log_context.open_log_file);
  }
}

static bool open_data_log(const char* filename, BYTE mode) {
  // close the previous log file
  close_data_log();

  // ensure the data log folder exists
  f_mkdir(DATA_LOG_DIR_PATH);

  FRESULT result = f_open(&g_data_log_context.open_log_file, filename, mode);
  if(result){
    LOGE(TAG, "f_open() for %s returned %u\n", filename, result);
    return false;
  }
  // write version 2 of the log file
  UINT bytes_written;
  uint8_t comp[] = {0x00, 0x02};
  f_write_nowait(&g_data_log_context.open_log_file, &(comp[0]), 2, &bytes_written);
  // write the eeg gain

  return true;
}

static bool open_data_log(){
  char log_fnum_buf[15];
  size_t datalog_uid = 0;

  // get log unique ID
#if 0
  // count the log files in the data log
  size_t num_log_files = CountObjectUnder(DATA_LOG_DIR_PATH);
  snprintf ( log_fnum_buf, sizeof(log_fnum_buf), "%d", num_log_files+1 );
#else
  // get previously used UID from settings file
  if ( getLogFileUID(log_fnum_buf, sizeof(log_fnum_buf)) ) {
    log_fnum_buf[sizeof(log_fnum_buf)-1] = '\0';
    datalog_uid = atoi(log_fnum_buf)+1;
  }else{
    datalog_uid = 0;
  }
#endif

  while(true){
    snprintf ( log_fnum_buf, sizeof(log_fnum_buf), "%d", datalog_uid );

    // create log file name
    char log_fname[MAX_PATH_LENGTH];
    size_t log_fsize = 0;
    log_fsize = str_append2(log_fname, log_fsize, DATA_LOG_DIR_PATH); // directory
    log_fsize = str_append2(log_fname, log_fsize, "/");               // path separator
    log_fsize = str_append2(log_fname, log_fsize, "log");             // log file name
    log_fsize = str_append2(log_fname, log_fsize, log_fnum_buf);      // log file number

  #if (defined(CONFIG_DATALOG_USE_TIME_STRING) && (CONFIG_DATALOG_USE_TIME_STRING > 0U))
    taskENTER_CRITICAL();
    if (g_data_log_context.datetime_size != 0) {
      log_fsize = str_append2(log_fname, log_fsize, "_");             // log file name
      log_fsize = str_append2(log_fname, log_fsize, g_data_log_context.datetime);   // log file number
    }
    taskEXIT_CRITICAL();
  #else
    char log_datetime[12];
    snprintf(log_datetime, sizeof(log_datetime), "%lu", rtc_get());
    log_fsize = str_append2(log_fname, log_fsize, "_");
    log_fsize = str_append2(log_fname, log_fsize, log_datetime);
  #endif

    log_fsize = str_append2(log_fname, log_fsize, ".bin");            // log file suffix

    // TODO: Delete the oldest log file?

  //  char* log_fname = (char*)"/log1.bin";
    // remove the old log file
    // f_unlink(log_fname);
    // create the new log file

    if( open_data_log(log_fname, FA_CREATE_NEW | FA_WRITE) ){
      // save the new uid
      setLogFileUID(log_fnum_buf);
      return true;
    }else{
      datalog_uid ++;
    }
  } // end while loop

}


bool open_eeg_comp_data_log(const char* log_filename_to_compress) {

  // create log file name
  char log_fname[MAX_PATH_LENGTH];
  size_t log_fsize = 0;
  log_fsize = str_append2(log_fname, log_fsize, DATA_LOG_DIR_PATH); // directory
  log_fsize = str_append2(log_fname, log_fsize, "/");               // path separator
  log_fsize = str_append2(log_fname, log_fsize, "c");             // "compressed" prefix
  log_fsize = str_append2(log_fname, log_fsize, log_filename_to_compress);      // source filename

  // Open file with FA_CREATE_ALWAYS to overwrite any previous
  // (possibly aborted) conversion.
  return open_data_log(log_fname, FA_CREATE_ALWAYS | FA_WRITE);
}
#endif

/*****************************************************************************/
// Thread-safe functions

void data_log_open()
{
  data_log_event_t event = {.type = DATA_LOG_EVENT_OPEN };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void data_log_close()
{
  data_log_event_t event = {.type = DATA_LOG_EVENT_CLOSE };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void data_log_set_time(char *datetime_string, size_t datetime_size) {
#if (defined(CONFIG_DATALOG_USE_TIME_STRING) && (CONFIG_DATALOG_USE_TIME_STRING > 0U))
  // In order for xMessageBufferSend to be task safe:
  // 1. It needs to be enclosed in a critical section
  // 2. The wait time needs to be 0.

  size_t datetime_size_corr = std::min(datetime_size + 1, (size_t) DATETIME_STRING_MAX_SIZE);

  taskENTER_CRITICAL();
  memcpy(g_data_log_context.datetime, datetime_string, datetime_size_corr);
  g_data_log_context.datetime[datetime_size_corr - 1] = '\0';
  g_data_log_context.datetime_size = datetime_size_corr - 1;
  taskEXIT_CRITICAL();
#endif
}

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

// start compression of existing file
void data_log_compress(char* log_filename_to_compress) {
  /* Just copy the filename into global context within a critical
     section, like we do for data_log_set_time().

     TODO: We should really be doing this with xMessageBufferSend(),
     but since we're dumping all message buffers out (instead of just
     the message corresponding to the event we're pulling out of the
     queue) in handle_state_opened() for DATA_LOG_EVENT_READY_TO_SEND,
     sending it with xMessageBufferSend() would corrupt an open log
     file if we call this while we're writing data.
  */

  if (strlen(log_filename_to_compress) + 1 <= sizeof(g_data_log_context.log_filename_to_compress)) {
    taskENTER_CRITICAL();
    memcpy(g_data_log_context.log_filename_to_compress, log_filename_to_compress, strlen(log_filename_to_compress));
    g_data_log_context.log_filename_to_compress[strlen(log_filename_to_compress)] = '\0';
    taskEXIT_CRITICAL();

    data_log_event_t event = {.type = DATA_LOG_EVENT_COMPRESS };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  } else {
    LOGE(TAG, "data_log_compress(): Error: Filename too long");
  }
}

// check compression status
// - sets is_compressing flag if still compressing, else false
// - sets result to last compression result (true on success)
// - sets stats flags to indicate progress
void data_log_compress_status(bool* is_compressing, bool* result, size_t* bytes_in_file, size_t* bytes_processed) {
  // Just copy the status from global context inside a critical section.
  // TODO: Would be better to do this using an atomic or sync primitive.
  taskENTER_CRITICAL();
  *is_compressing = (g_data_log_context.state == DATA_LOG_STATE_COMPRESSING);
  *result = g_data_log_context.log_compression_successful;
  data_log_parse_stats(bytes_in_file, bytes_processed);
  taskEXIT_CRITICAL();
}

// abort compression (no effect if not currently compressing)
void data_log_compress_abort() {
  data_log_parse_stop();
}

#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

#if (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))
/*
 * Called to write general data to the data log
 */
void data_log_write(const uint8_t *scratch, uint32_t scratch_size){
  // encode into COBS
#if (defined(CONFIG_DATALOG_USE_COBSR_RLE0) && (CONFIG_DATALOG_USE_COBSR_RLE0 > 0U))
  cobsr_encode_result result = cobsr_rle0_encode(&(g_data_log_context.dst_scratch[0]), DST_SCRATCH_BUFFER_SIZE, scratch, scratch_size);
  size_t dst_size = result.out_len;
#else
  size_t dst_size = COBS::encode(scratch, scratch_size, &(g_data_log_context.dst_scratch[0]));
#endif

//  LOGV(TAG,"data_log_write, scratch_size: %lu, cobs_dst_size: %u", scratch_size, dst_size);

  // write packet marker
  g_data_log_context.dst_scratch[dst_size] = 0;

  uint8_t *input = &(g_data_log_context.dst_scratch[0]);
  uint32_t input_size = dst_size+1;

  size_t comp_sz = HSE_SCRATCH_BUFFER_SIZE;
  uint8_t *comp = &(g_data_log_context.hse_scratch[0]);

  size_t count = 0;
  uint32_t sunk = 0;
  uint32_t polled = 0;
  while (sunk < input_size) {
      //ASSERT(heatshrink_encoder_sink(&hse, &input[sunk], input_size - sunk, &count) >= 0);
      heatshrink_encoder_sink(&(g_data_log_context.hse), &input[sunk], input_size - sunk, &count);
      sunk += count;

      HSE_poll_res pres;
      do {                    /* "turn the crank" */
          pres = heatshrink_encoder_poll(&(g_data_log_context.hse), &comp[polled], comp_sz - polled, &count);
          polled += count;
      } while (pres == HSER_POLL_MORE);
  }

  if(polled > 0){
    UINT bytes_written;
    f_write_nowait(&g_data_log_context.open_log_file, &(comp[0]), polled, &bytes_written);
    // TODO: What happens if f_write fails?
  }
}

/*
 * Called to finish heatshrink compression before closing the data log
 */
static void data_log_finish_write(){
  // end heatshrink compression
  HSE_finish_res fres = heatshrink_encoder_finish(&(g_data_log_context.hse));

  size_t comp_sz = HSE_SCRATCH_BUFFER_SIZE;
  uint8_t *comp = &(g_data_log_context.hse_scratch[0]);

  size_t count = 0;
  uint32_t polled = 0;
  if(fres == HSER_FINISH_MORE){
    HSE_poll_res pres;
    do {                    /* "turn the crank" */
      pres = heatshrink_encoder_poll(&(g_data_log_context.hse), &comp[polled], comp_sz - polled, &count);
      polled += count;
    } while (pres == HSER_POLL_MORE);
  }

  if (polled > 0) {
    UINT bytes_written;
    // Make sure the file is open before writing to it.
    // This is needed because this routine is called upon init
    // and the file is invalid. The result is a bogus status
    // in the writer task for the next write.
    if(f_is_open(&g_data_log_context.open_log_file)) {
      f_write_nowait(&g_data_log_context.open_log_file, &(comp[0]), polled, &bytes_written);
      // TODO: What happens if f_write_nowait fails?
    }
  }
}
#endif // (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))

/*****************************************************************************/
// State machine

static void
log_event(data_log_event_t *event)
{
  switch (event->type) {
    case DATA_LOG_EVENT_READY_TO_SEND:
      // ignore this data
      break;
    case DATA_LOG_EEG_DATA:
      // ignore this data
      break;
    case DATA_LOG_INST_DATA:
      // ignore this data
      break;

    default:
      LOGV(TAG, "[%s] Event: %s",
          data_log_state_name(g_data_log_context.state),
          data_log_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(data_log_event_t *event)
{
  switch (event->type) {
    case DATA_LOG_EVENT_READY_TO_SEND:
      // ignore this data
      break;

    default:
      LOGD(TAG, "[%s] Ignored Event: %s",
          data_log_state_name(g_data_log_context.state),
          data_log_event_type_name(event->type));
      break;
  }
}

static void
set_state(data_log_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]",
      data_log_state_name(g_data_log_context.state),
      data_log_state_name(state));

  g_data_log_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  data_log_event_t event = {.type = DATA_LOG_EVENT_ENTER_STATE };
  handle_event(&event);
}


//
// Event handlers for the various application states:
//

static void
handle_state_closed(data_log_event_t *event)
{
  switch (event->type) {
    case DATA_LOG_EVENT_ENTER_STATE:
      set_file_ready(false);
#if (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))
      // finish compression and writing
      data_log_finish_write();
      // close the log
      close_data_log();
#endif // (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))
      break;

    case DATA_LOG_EVENT_OPEN:
      set_state(DATA_LOG_STATE_OPENED);
      break;

    case DATA_LOG_EVENT_READY_TO_SEND:
      break;

    case DATA_LOG_EVENT_COMPRESS:
      // TODO: Save event message as log file to compress
      set_state(DATA_LOG_STATE_COMPRESSING);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_opened(data_log_event_t *event)
{
  switch (event->type) {
    case DATA_LOG_EVENT_ENTER_STATE:
#if (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))
      if ( open_data_log() ){
        // reset the pack buffers
        data_log_eeg_reset();
        data_log_inst_reset();
        data_log_stim_reset();
        // initialize the heatshrink encoder
        heatshrink_encoder_reset(&(g_data_log_context.hse));
        set_file_ready(true);
      }else{
        LOGE(TAG, "Failed to open data log");
        set_state(DATA_LOG_STATE_CLOSED);
      }
#endif // (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))
      break;

    case DATA_LOG_EVENT_CLOSE:
      set_state(DATA_LOG_STATE_CLOSED);
      break;

    case DATA_LOG_EVENT_READY_TO_SEND: 
    {
#if (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))
//        LOGV(TAG,"write msg_type: %d", event->msg[0]);
          // compress & write
          data_log_write(event->msg, event->msg_size);
#endif // (defined(ENABLE_DATA_LOG_SAVE_TO_LOFFILE) && (ENABLE_DATA_LOG_SAVE_TO_LOFFILE > 0U))
      break;
    }

    case DATA_LOG_EEG_DATA:
      handle_eeg_data(event->msg, event->msg_size);
      break;

    case DATA_LOG_INST_DATA:
      handle_inst_data(event->msg, event->msg_size);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_compressing(data_log_event_t *event)
{
  switch (event->type) {
    case DATA_LOG_EVENT_ENTER_STATE:
      // initialize the heatshrink encoder
      heatshrink_encoder_reset(&(g_data_log_context.hse));
#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))
      g_data_log_context.log_compression_successful =
        compress_log_eeg(g_data_log_context.log_filename_to_compress);
#else
      g_data_log_context.log_compression_successful = false;
#endif
      set_state(DATA_LOG_STATE_CLOSED);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_event(data_log_event_t *event)
{
  // handle stateless events
  switch(event->type){
  default:
    break;
  }

  // handle state dependent events
  switch (g_data_log_context.state) {
    case DATA_LOG_STATE_CLOSED:
      handle_state_closed(event);
      break;

    case DATA_LOG_STATE_OPENED:
      handle_state_opened(event);
      break;

    case DATA_LOG_STATE_COMPRESSING:
      handle_state_compressing(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown data log state: %d", (int) g_data_log_context.state);
      break;
  }
}

void
data_log_pretask_init(void)
{
  configASSERT(sizeof(data_log_event_type_t) == 1);

  // Allocate buffer memory
#if (defined(DATA_LOG_USE_LOCAL_MEMORY_MANAGER) && (DATA_LOG_USE_LOCAL_MEMORY_MANAGER > 0U))
  mm_rtos_init(&g_mm_rtos, g_mm_buf, sizeof(g_mm_buf) );
#endif // (defined(DATA_LOG_USE_LOCAL_MEMORY_MANAGER) && (DATA_LOG_USE_LOCAL_MEMORY_MANAGER > 0U))

  // Any pre-scheduler init goes here.
  memset(&g_data_log_context, 0, sizeof(g_data_log_context));

  // Create logging enabled semaphore
  g_data_log_context.file_ready_sem = xSemaphoreCreateMutexStatic( &(g_data_log_context.file_ready_sem_buf) );
  g_data_log_context.file_ready=false;

  // Create the event queue before the scheduler starts. Avoids race conditions.
//  memset(g_event_queue_array, 0, sizeof(g_event_queue_array));
  g_event_queue = xQueueCreateStatic(DATA_LOG_EVENT_QUEUE_SIZE, sizeof(data_log_event_t), g_event_queue_array, &g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "data_log_event_queue");

  data_log_eeg_init();
  data_log_inst_init();
}


static void
task_init()
{
  // Any post-scheduler init goes here.
  set_state(DATA_LOG_STATE_CLOSED);

  // ensure the data log folder exists
  f_mkdir(DATA_LOG_DIR_PATH);

  LOGV(TAG, "Task launched. Entering event loop.");
}


void
data_log_task(void *ignored)
{
  task_init();

  data_log_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

    if(event.msg != NULL){
      dl_free(event.msg);
    }
  }
}


#else // (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))


void data_log_pretask_init(void){}
void data_log_task(void *ignored){}
void data_log_open(){}
void data_log_close(){}

void data_log_set_time(char *datetime_string, size_t datetime_size){}

void* dl_malloc_if_file_ready(size_t size){ return NULL; }
void send_data(uint8_t *scratch, uint32_t scratch_size, data_log_event_type_t event_type, TickType_t xTicksToWait){}
void send_data(uint8_t *scratch, uint32_t scratch_size, TickType_t xTicksToWait){}
void send_data(DLBuffer *dlbuf, TickType_t xTicksToWait){}

#endif // (defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK > 0U))


/*****************************************************************************/
// Miscellaneous helper functions

void add_to_buffer(uint8_t *buf, size_t buf_size, size_t &offset, void* data, size_t data_size){
  configASSERT( (offset + data_size) <= buf_size );
  memcpy( buf+offset, data, data_size );
  offset += data_size;
}

void add_to_buffer(uint8_t *buf, size_t buf_size, size_t &offset, uint8_t data){
  add_to_buffer(buf, buf_size, offset, (void*) &data, 1);
}
