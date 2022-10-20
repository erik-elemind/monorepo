/*
 * eeg_reader.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Aug, 2020
 * Author:  David Wang
 */

#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "message_buffer.h"

#include "utils.h"
#include "config.h"
#include "config_tracealyzer_isr.h"

#include "ads129x_command.h"
#include "ads129x.h"

#include "eeg_processor.h"
#include "eeg_processing.h"
#include "interpreter.h"


#include "data_log.h"
#include "erp.h"
#include "memman_rtos.h"


#if (defined(ENABLE_EEG_PROCESSOR_TASK) && (ENABLE_EEG_PROCESSOR_TASK > 0U))

#define EEG_PROCESSOR_EVENT_QUEUE_SIZE (100) // 100
#define EEG_PROCESSOR_EVENT_MEMORY_SIZE (1000)

static const char *TAG = "eeg_processor";  // Logging prefix for this module

//
// Task events:
//
typedef enum
{
  EEG_PROCESSOR_EVENT_ENTER_STATE,	// (used for state transitions)
  EEG_PROCESSOR_EVENT_INIT,
  EEG_PROCESSOR_EVENT_EEG_MSG_AVAIL,
  EEG_PROCESSOR_EVENT_START_QUALITY_CHECK,
  EEG_PROCESSOR_EVENT_STOP_QUALITY_CHECK,
  EEG_PROCESSOR_EVENT_START_BLINK_TEST,
  EEG_PROCESSOR_EVENT_STOP_BLINK_TEST,
  EEG_PROCESSOR_EVENT_ENABLE_LINE_FILTERS,
  EEG_PROCESSOR_EVENT_ENABLE_AZ_FILTERS,
  EEG_PROCESSOR_EVENT_CONFIG_LINE_FILTER,
  EEG_PROCESSOR_EVENT_CONFIG_AZ_FILTER,
  EEG_PROCESSOR_EVENT_CONFIG_ECHT,
  EEG_PROCESSOR_EVENT_SET_ECHT_CHANNEL,
  EEG_PROCESSOR_EVENT_SET_ECHT_PHASE,
  EEG_PROCESSOR_EVENT_START_ECHT,
  EEG_PROCESSOR_EVENT_STOP_ECHT,
  EEG_PROCESSOR_EVENT_ENABLE_ALPHA_THRESH,
  EEG_PROCESSOR_EVENT_CONFIG_ALPHA_THRESH,

} eeg_processor_event_type_t;

typedef struct{
  bool enable_filter;
} enable_filter_t;

typedef struct {
  int  filter_order;
  double filter_cutfreq;
} config_filter_t;

typedef struct {
  int fft_size;     // = 128;
  int filter_order; // = 2;
  float center_freq; // 10-12
  float low_freq;   // = center_freq-(center_freq/4);
  float high_freq;  // = center_freq+(center_freq/4);
  float input_scale;// = -1;
  float sample_freq;// = 250;
} config_echt_t;

typedef struct {
  float min_phase_rad;
  float max_phase_rad;
} echt_phase_t;

typedef struct{
  eeg_channel_t echt_channel_number;
} echt_channel_number_t;

typedef struct{
  bool enable;
  float dur1_sec;
  float dur2_sec;
  float timedLockOut_sec;
  float minRMSPower_uV;
  float alphaThr_dB;
} alpha_switch_t;

// Events are passed to the g_event_queue with an optional 
// void *user_data pointer (which may be NULL).
typedef struct
{
  eeg_processor_event_type_t type;
  void *user_data;
} eeg_processor_event_t;

//
// State machine states:
//
typedef enum
{
  EEG_PROCESSOR_STATE_STANDBY,
} eeg_processor_state_t;

//
// Global context data:
//
typedef struct
{
  eeg_processor_state_t state;

  EEGProcessing eegp;

} eeg_processor_context_t;

static eeg_processor_context_t g_eeg_processor_context;
static TaskHandle_t g_eeg_processor_task_handle = NULL;

// Global event queue and handler:
static uint8_t g_event_queue_array[EEG_PROCESSOR_EVENT_QUEUE_SIZE*sizeof(eeg_processor_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(eeg_processor_event_t *event);

// Global memory
static uint8_t* g_event_memory_buf[EEG_PROCESSOR_EVENT_MEMORY_SIZE];
static mm_rtos_t g_event_memory;

#define PROC_MALLOC(X) ((X*)mm_rtos_malloc(&g_event_memory,sizeof(X),portMAX_DELAY))


static void eeg_processor_receive_eeg_data(void* data);


// For logging and debug:
static const char *
eeg_processor_state_name(eeg_processor_state_t state)
{
  switch (state) {
    case EEG_PROCESSOR_STATE_STANDBY:  return "EEG_PROCESSOR_STATE_STANDBY";
    default:
      break;
  }
  return "EEG_READER_STATE UNKNOWN";
}


static const char *
eeg_processor_event_type_name(eeg_processor_event_type_t event_type)
{
  switch (event_type) {
    case EEG_PROCESSOR_EVENT_ENTER_STATE: return "EEG_PROCESSOR_EVENT_ENTER_STATE";
    case EEG_PROCESSOR_EVENT_INIT:         return "EEG_PROCESSOR_EVENT_INIT";
    case EEG_PROCESSOR_EVENT_EEG_MSG_AVAIL:      return "EEG_PROCESSOR_EVENT_EEG_MSG_AVAIL";
    case EEG_PROCESSOR_EVENT_START_QUALITY_CHECK: return "EEG_PROCESSOR_EVENT_START_QUALITY_CHECK";
    case EEG_PROCESSOR_EVENT_STOP_QUALITY_CHECK: return "EEG_PROCESSOR_EVENT_STOP_QUALITY_CHECK";
    case EEG_PROCESSOR_EVENT_START_BLINK_TEST:    return "EEG_PROCESSOR_EVENT_START_BLINK_TEST";
    case EEG_PROCESSOR_EVENT_STOP_BLINK_TEST:    return "EEG_PROCESSOR_EVENT_STOP_BLINK_TEST";
    case EEG_PROCESSOR_EVENT_ENABLE_LINE_FILTERS: return "EEG_PROCESSOR_EVENT_ENABLE_LINE_FILTERS";
    case EEG_PROCESSOR_EVENT_ENABLE_AZ_FILTERS:  return "EEG_PROCESSOR_EVENT_ENABLE_AZ_FILTERS";
    case EEG_PROCESSOR_EVENT_CONFIG_LINE_FILTER: return "EEG_PROCESSOR_EVENT_CONFIG_LINE_FILTER";
    case EEG_PROCESSOR_EVENT_CONFIG_AZ_FILTER:   return "EEG_PROCESSOR_EVENT_CONFIG_AZ_FILTER";
    case EEG_PROCESSOR_EVENT_CONFIG_ECHT:        return "EEG_PROCESSOR_EVENT_CONFIG_ECHT";
    case EEG_PROCESSOR_EVENT_SET_ECHT_CHANNEL:   return "EEG_PROCESSOR_EVENT_SET_ECHT_CHANNEL";
    case EEG_PROCESSOR_EVENT_SET_ECHT_PHASE:     return "EEG_PROCESSOR_EVENT_SET_ECHT_PHASE";
    case EEG_PROCESSOR_EVENT_START_ECHT:         return "EEG_PROCESSOR_EVENT_START_ECHT";
    case EEG_PROCESSOR_EVENT_STOP_ECHT:          return "EEG_PROCESSOR_EVENT_STOP_ECHT";
    case EEG_PROCESSOR_EVENT_ENABLE_ALPHA_THRESH: return "EEG_PROCESSOR_EVENT_ENABLE_ALPHA_SWITCH";
    case EEG_PROCESSOR_EVENT_CONFIG_ALPHA_THRESH: return "EEG_PROCESSOR_EVENT_CONFIG_ALPHA_SWITCH";
    default:
      break;
  }
  return "EEG_PROCESSOR_EVENT UNKNOWN";
}

void eeg_processor_init(void){
  eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_INIT, .user_data = NULL};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_processor_start_quality_check(void){
  eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_START_QUALITY_CHECK, .user_data = NULL};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_processor_stop_quality_check(void){
  eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_STOP_QUALITY_CHECK, .user_data = NULL};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_processor_start_blink_test(void){
  eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_START_BLINK_TEST, .user_data = NULL};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_processor_stop_blink_test(void){
  eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_STOP_BLINK_TEST, .user_data = NULL};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_processor_enable_line_filters(bool enable){
  enable_filter_t* data = PROC_MALLOC(enable_filter_t);
  if(data!=NULL){
    data->enable_filter = enable;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_ENABLE_LINE_FILTERS, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_enable_az_filters(bool enable){
  enable_filter_t* data = PROC_MALLOC(enable_filter_t);
  if(data!=NULL){
    data->enable_filter = enable;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_ENABLE_AZ_FILTERS, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_config_line_filter(int order, double cutfreq){
  config_filter_t* data = PROC_MALLOC(config_filter_t);
  if(data!=NULL){
    data->filter_order = order;
    data->filter_cutfreq = cutfreq;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_CONFIG_LINE_FILTER, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_config_az_filter(int order, double cutfreq){
  config_filter_t* data = PROC_MALLOC(config_filter_t);
  if(data!=NULL){
    data->filter_order = order;
    data->filter_cutfreq = cutfreq;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_CONFIG_AZ_FILTER, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_config_echt(int fft_size, int filter_order, float center_freq, float low_freq, float high_freq, float input_scale, float sample_freq){
  config_echt_t* data = PROC_MALLOC(config_echt_t);
  if(data!=NULL){
    data->fft_size = fft_size;
    data->filter_order = filter_order;
    data->center_freq = center_freq;
    data->low_freq = low_freq;
    data->high_freq = high_freq;
    data->input_scale = input_scale;
    data->sample_freq = sample_freq;
    eeg_processor_event_t event = { .type = EEG_PROCESSOR_EVENT_CONFIG_ECHT, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_config_echt_simple(float center_freq){
  config_echt_t* data = PROC_MALLOC(config_echt_t);
  if(data!=NULL){
    data->fft_size = 128;
    data->filter_order = 2;
    data->center_freq = center_freq;
    data->low_freq = center_freq-(center_freq/4);
    data->high_freq = center_freq+(center_freq/4);
    data->input_scale = -1;
    data->sample_freq = 250;
    eeg_processor_event_t event = { .type = EEG_PROCESSOR_EVENT_CONFIG_ECHT, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_set_echt_channel(eeg_channel_t channel_number){
  echt_channel_number_t* data = PROC_MALLOC(echt_channel_number_t);
  if(data!=NULL){
    data->echt_channel_number = channel_number;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_SET_ECHT_CHANNEL, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_set_echt_min_max_phase(float min_phase_rad, float max_phase_rad){
  echt_phase_t* data = PROC_MALLOC(echt_phase_t);
  if(data!=NULL){
    data->min_phase_rad = min_phase_rad;
    data->max_phase_rad = max_phase_rad;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_SET_ECHT_PHASE, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_start_echt(){
  eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_START_ECHT, .user_data = NULL};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_processor_stop_echt(){
  eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_STOP_ECHT, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_processor_enable_alpha_switch(bool enable){
  alpha_switch_t* data = PROC_MALLOC(alpha_switch_t);
  if(data!=NULL){
    data->enable = enable;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_ENABLE_ALPHA_THRESH, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void eeg_processor_config_alpha_switch(float dur1_sec, float dur2_sec, float timedLockOut_sec, float minRMSPower_uV, float alphaThr_dB){
  alpha_switch_t* data = PROC_MALLOC(alpha_switch_t);
  if(data!=NULL){
    data->dur1_sec = dur1_sec;
    data->dur2_sec = dur2_sec;
    data->timedLockOut_sec = timedLockOut_sec;
    data->minRMSPower_uV = minRMSPower_uV;
    data->alphaThr_dB = alphaThr_dB;
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_CONFIG_ALPHA_THRESH, .user_data = data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}


static void
log_event(eeg_processor_event_t *event)
{
  switch (event->type) {
    case EEG_PROCESSOR_EVENT_EEG_MSG_AVAIL:
      // do nothing -- suppress printing EEG MSG events
      break;
    default:
      LOGV(TAG, "[%s] Event: %s", eeg_processor_state_name(g_eeg_processor_context.state), eeg_processor_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(eeg_processor_event_t *event)
{
  switch(event->type){
    case EEG_PROCESSOR_EVENT_EEG_MSG_AVAIL:
      // do nothing -- suppress printing EEG MSG events
      break;
    default:
      LOGD(TAG, "[%s] Ignored Event: %s", eeg_processor_state_name(g_eeg_processor_context.state), eeg_processor_event_type_name(event->type));
      break;
  }
}

static void
handle_event(eeg_processor_event_t *event)
{
  // handle stateless events
  switch (event->type) {
  case EEG_PROCESSOR_EVENT_EEG_MSG_AVAIL:
    eeg_processor_receive_eeg_data(event->user_data);
    return;

  case EEG_PROCESSOR_EVENT_INIT:
    // initialize eeg processing
    g_eeg_processor_context.eegp.init();
    return;

  case EEG_PROCESSOR_EVENT_START_BLINK_TEST:
    g_eeg_processor_context.eegp.start_blink_test();
    return;

  case EEG_PROCESSOR_EVENT_STOP_BLINK_TEST:
    g_eeg_processor_context.eegp.stop_blink_test();
    return;

  case EEG_PROCESSOR_EVENT_START_QUALITY_CHECK:
    g_eeg_processor_context.eegp.start_electrode_quality_test();
    return;

  case EEG_PROCESSOR_EVENT_STOP_QUALITY_CHECK:
    g_eeg_processor_context.eegp.stop_electrode_quality_test();
    return;

  case EEG_PROCESSOR_EVENT_ENABLE_LINE_FILTERS:
  {
    enable_filter_t* data = (enable_filter_t*)event->user_data;
    g_eeg_processor_context.eegp.line_filter_enable( data->enable_filter );
    return;
  }
  case EEG_PROCESSOR_EVENT_ENABLE_AZ_FILTERS:
  {
    enable_filter_t* data = (enable_filter_t*)event->user_data;
    g_eeg_processor_context.eegp.az_filter_enable( data->enable_filter );
    return;
  }
  case EEG_PROCESSOR_EVENT_CONFIG_LINE_FILTER:
  {
    config_filter_t* data = (config_filter_t*)event->user_data;
    g_eeg_processor_context.eegp.line_filter_config(
        data->filter_order,
        data->filter_cutfreq );
    return;
  }
  case EEG_PROCESSOR_EVENT_CONFIG_AZ_FILTER:
  {
    config_filter_t* data = (config_filter_t*)event->user_data;
    g_eeg_processor_context.eegp.az_filter_config(
        data->filter_order,
        data->filter_cutfreq );
    return;
  }
#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))
  case EEG_PROCESSOR_EVENT_CONFIG_ECHT:
  {
    config_echt_t* data = (config_echt_t*)event->user_data;
    g_eeg_processor_context.eegp.echt_config(
        data->fft_size,
        data->filter_order,
        data->center_freq,
        data->low_freq,
        data->high_freq,
        data->input_scale,
        data->sample_freq);
    return;
  }
  case EEG_PROCESSOR_EVENT_SET_ECHT_CHANNEL:
  {
    echt_channel_number_t* data = (echt_channel_number_t*)event->user_data;
    g_eeg_processor_context.eegp.echt_set_channel(data->echt_channel_number);
    return;
  }
  case EEG_PROCESSOR_EVENT_SET_ECHT_PHASE:
  {
    echt_phase_t* data = (echt_phase_t*)event->user_data;
    g_eeg_processor_context.eegp.echt_set_min_max_phase(data->min_phase_rad, data->max_phase_rad);
    return;
  }
  case EEG_PROCESSOR_EVENT_START_ECHT:
    g_eeg_processor_context.eegp.echt_enable(true);
    return;

  case EEG_PROCESSOR_EVENT_STOP_ECHT:
    g_eeg_processor_context.eegp.echt_enable(false);
    return;

  case EEG_PROCESSOR_EVENT_ENABLE_ALPHA_THRESH:
  {
    alpha_switch_t* data = (alpha_switch_t*)event->user_data;
    g_eeg_processor_context.eegp.channel_switch_enable(data->enable);
    return;
  }
  case EEG_PROCESSOR_EVENT_CONFIG_ALPHA_THRESH:
  {
    alpha_switch_t* data = (alpha_switch_t*)event->user_data;
    g_eeg_processor_context.eegp.set_params(
        data->dur1_sec,
        data->dur2_sec,
        data->timedLockOut_sec,
        data->minRMSPower_uV,
        data->alphaThr_dB);
    return;
  }
#endif

  default:
    break;
  }

  switch (g_eeg_processor_context.state) {
    case EEG_PROCESSOR_STATE_STANDBY:
      log_event_ignored(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown eeg_processor state: %d", (int) g_eeg_processor_context.state);
      break;
  }

}

void
eeg_processor_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(EEG_PROCESSOR_EVENT_QUEUE_SIZE,sizeof(eeg_processor_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "eeg_processor_event_queue");

  // Create the event memory
  mm_rtos_init( &g_event_memory, g_event_memory_buf, sizeof(g_event_memory_buf) );

  // Design filters
  g_eeg_processor_context.eegp.filters_init();

#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))

  // Configure ECHT to operate at a given sampling frequency, tracking a 5Hz oscillation being sampled at.
  // TODO: Move these phase parameters to one global struct
  int fft_size = 128;
  int filter_order = 2;
  float center_freq = 10;
  float low_freq = center_freq-(center_freq/4);
  float high_freq = center_freq+(center_freq/4);
  float input_scale = -1;
  float sample_freq = 250;
  g_eeg_processor_context.eegp.echt_config(fft_size, filter_order, center_freq, low_freq, high_freq, input_scale, sample_freq);

  // set echt channel number
  g_eeg_processor_context.eegp.echt_set_init_channel(EEG_FP1);
#endif // end defined(ECHT_ENABLE)

}


static void
task_init()
{
  // Any post-scheduler init goes here.
  g_eeg_processor_task_handle = xTaskGetCurrentTaskHandle();

  LOGV(TAG, "Task launched. Entering event loop.");
}

void
eeg_processor_task(void *ignored)
{

  task_init();

  eeg_processor_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

    // free the malloc'd memory
    if( event.user_data!= NULL){
      mm_rtos_free(&g_event_memory, event.user_data);
    }
  }
}

uint8_t* eeg_processor_send_eeg_data_open_from_isr(size_t data_len, BaseType_t *pxHigherPriorityTaskWoken){
  configASSERT(data_len == EEG_MSG_LEN);
  uint8_t* malloc_data = NULL;
  if (g_eeg_processor_task_handle != NULL){
    malloc_data = ((uint8_t*) mm_rtos_malloc_from_isr(&g_event_memory,EEG_MSG_LEN,pxHigherPriorityTaskWoken));
  }
  return malloc_data;
}

void eeg_processor_send_eeg_data_close_from_isr(uint8_t* data, size_t data_len, BaseType_t *pxHigherPriorityTaskWoken) {
  configASSERT(data_len == EEG_MSG_LEN);
  if( data != NULL ){
    eeg_processor_event_t event = {.type = EEG_PROCESSOR_EVENT_EEG_MSG_AVAIL, .user_data = data};
    xQueueSendFromISR(g_event_queue, &event, pxHigherPriorityTaskWoken);
  }
}

static void eeg_processor_receive_eeg_data(void* data) {
  uint8_t* eegRxData = (uint8_t*) data;
  static uint8_t sample_counter = 0;

  if (data == NULL) {
    return;
  }

#if (defined(USE_3CHANNEL_EEG) && (USE_3CHANNEL_EEG > 0U))
  ads129x_frontal_sample f_sample;
  ads_decode_frontal_sample(eegRxData, EEG_MSG_LEN, &f_sample);
#else
  ads129x_frontal_sample f_sample;
  {
    ads129x_sample sample;
    ads_decode_sample(eegRxData, sizeof( eegRxData ), &sample);
    // rearrange sampled data relative to electrodes
//    f_sample.timestamp_ms = sample.timestamp_ms;
    f_sample.sample_number = sample.sample_number;
//    f_sample.status = sample.status;
    f_sample.eeg_channels[EEG_FP1] = sample.eeg_channels[EEG_CH2]; // fp1
    f_sample.eeg_channels[EEG_FPZ] = sample.eeg_channels[EEG_CH4]; // fpz
    f_sample.eeg_channels[EEG_FP2] = sample.eeg_channels[EEG_CH1]; // fp2

//    eeg_quality_compute( &sample );
  }
#endif

  //ToDo: Disable once testing
  sample_counter++;
  if(sample_counter == 200)
  {
	  sample_counter = 0;
	  LOGV(TAG, "eeg %ld, %ld, %ld", f_sample.eeg_channels[0], f_sample.eeg_channels[1], f_sample.eeg_channels[2]);
  }

#if 1
  g_eeg_processor_context.eegp.process(&f_sample);
#else
  stream_eeg(&f_sample);
#endif

  erp_set_eeg_sample_number(f_sample.sample_number);
}

#else /*  (defined(ENABLE_EEG_PROCESSOR_TASK) && (ENABLE_EEG_PROCESSOR_TASK > 0U)) */

void eeg_processor_pretask_init(void){}
void eeg_processor_task(void *ignored){}

void eeg_processor_send_eeg_data_from_isr(uint8_t* data, size_t data_len){
#if (defined(USE_3CHANNEL_EEG) && (USE_3CHANNEL_EEG > 0U))
  ads129x_frontal_sample f_sample;
  ads_decode_frontal_sample(data, data_len, &f_sample);
  data_log_eeg(&f_sample);
#endif
}
void eeg_processor_init(void){}

void eeg_processor_start_quality_check(void){}
void eeg_processor_stop_quality_check(void){}
void eeg_processor_start_blink_test(void){}
void eeg_processor_stop_blink_test(void){}
void eeg_processor_enable_line_filters(bool enable){}
void eeg_processor_enable_az_filters(bool enable){}
void eeg_processor_config_line_filter(int order, double cutfreq){}
void eeg_processor_config_az_filter(int order, double cutfreq){}
void eeg_processor_config_echt(int fftSize, int filtOrder, float centerFreq, float lowFreq, float highFreq, float inputScale, float sampFreqWithDrop){}
void eeg_processor_config_echt_simple(float center_freq){}
void eeg_processor_set_echt_channel(eeg_channel_t channel_number){}
void eeg_processor_set_echt_min_max_phase(float min_phase_rad, float max_phase_rad){}
void eeg_processor_start_echt(){}
void eeg_processor_stop_echt(){}
void eeg_processor_enable_alpha_switch(bool enable){}
void eeg_processor_config_alpha_switch(float dur1_sec, float dur2_sec, float timedLockOut_sec, float minRMSPower_uV, float alphaThr_dB){}

#endif /*  (defined(ENABLE_EEG_PROCESSOR_TASK) && (ENABLE_EEG_PROCESSOR_TASK > 0U)) */




