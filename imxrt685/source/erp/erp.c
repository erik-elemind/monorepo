/*
 * erp.cpp
 *
 *  Created on: Mar 18, 2022
 *      Author: david
 */

#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "audio_task.h"

#include "loglevels.h"
#include "erp.h"
#include "data_log.h"

#if (defined(ENABLE_ERP_TASK) && (ENABLE_ERP_TASK > 0U))

#define ERP_EVENT_QUEUE_SIZE 10

static const char *TAG = "erp"; // Logging prefix for this module

//
// Application events (high-level events for business logic):
//
typedef enum
{
  ERP_EVENT_ENTER_STATE,    // (used for state transitions)
  ERP_EVENT_START,
  ERP_EVENT_STOP,
  ERP_EVENT_PULSE_TIMEOUT,
  ERP_EVENT_ISI_TIMEOUT,
} erp_event_type_t;


// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  erp_event_type_t type;
  union{
    struct {
      uint32_t num_trials;
      uint32_t pulse_dur_ms;
      uint32_t isi_ms;
      uint32_t jitter_ms;
      uint8_t volume;
    } erp;
  } data;
} erp_event_t;

//
// Application state machine states:
//
typedef enum
{
  ERP_STATE_INVALID = 0,
  ERP_STATE_STANDBY,
  ERP_STATE_RUNNING,
} erp_state_t;

//
// Global application data (any business logic variables go here).
//
typedef struct
{
  erp_state_t state;

  TimerHandle_t pulse_timer_handle;
  StaticTimer_t pulse_timer_struct;
  TimerHandle_t isi_timer_handle;
  StaticTimer_t isi_timer_struct;

  uint32_t erp_num_trials;
  uint32_t erp_pulse_dur_ms;
  uint32_t erp_isi_ms;
  uint32_t erp_jitter_ms;
  uint8_t erp_volume;
  uint32_t erp_count;

  unsigned long sample_number;
} erp_context_t;

//
// Global variables
//

static erp_context_t g_erp_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[ERP_EVENT_QUEUE_SIZE*sizeof(erp_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(erp_event_t *event);

// For logging and debug:
static const char *
erp_state_name(erp_state_t state)
{
  switch (state) {
  case ERP_STATE_STANDBY: return "ERP_STATE_STANDBY";
  case ERP_STATE_RUNNING: return "ERP_STATE_RUNNING";

    default:
      break;
  }
  return "ERP_STATE UNKNOWN";
}

static const char *
erp_event_type_name(erp_event_type_t event_type)
{
  switch (event_type) {
    case ERP_EVENT_ENTER_STATE:    return "ERP_EVENT_ENTER_STATE";
    case ERP_EVENT_START:          return "ERP_EVENT_START";
    case ERP_EVENT_STOP:           return "ERP_EVENT_STOP";
    case ERP_EVENT_PULSE_TIMEOUT:  return "ERP_EVENT_PULSE_TIMEOUT";
    case ERP_EVENT_ISI_TIMEOUT:    return "ERP_EVENT_ISI_TIMEOUT";

    default:
      break;
  }
  return "ERP_EVENT UNKNOWN";
}


//
// Global function definitions
//

void
erp_event_start(uint32_t num_trials, uint32_t pulse_dur_ms, uint32_t isi_ms, uint32_t jitter_ms, uint8_t volume){
  erp_event_t event = {.type = ERP_EVENT_START,
      .data = { .erp = {.num_trials=num_trials, .pulse_dur_ms=pulse_dur_ms, .isi_ms=isi_ms, .jitter_ms=jitter_ms, .volume = volume }}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
erp_event_stop(){
  erp_event_t event = {.type = ERP_EVENT_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void erp_set_eeg_sample_number(unsigned long sample_number){
  // TODO: Check that the following assignment operation is atomic
  g_erp_context.sample_number = sample_number;
}

static void
log_event(erp_event_t *event)
{
  switch (event->type) {
    default:
      LOGV(TAG, "[%s] Event: %s", erp_state_name(g_erp_context.state), erp_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(erp_event_t *event)
{
  switch (event->type) {
    default:
       LOGD(TAG, "[%s] Ignored Event: %s", erp_state_name(g_erp_context.state), erp_event_type_name(event->type));
       break;
  }
}

static void
set_state(erp_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]", erp_state_name(g_erp_context.state), erp_state_name(state));

  g_erp_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  erp_event_t event = { ERP_EVENT_ENTER_STATE };
  handle_event(&event);
}

static void
restart_pulse_timer(uint32_t timeout_ms)
{
  // xTimerChangePeriod will start timer if it's not running already
  if (xTimerChangePeriod(g_erp_context.pulse_timer_handle,
      pdMS_TO_TICKS(timeout_ms), 0) == pdFAIL) {
    LOGE(TAG, "Unable to start pulse timer!");
  }
}

static void
stop_pulse_timer(void)
{
  if (xTimerStop(g_erp_context.pulse_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to stop pulse timer!");
  }
}

static void
restart_isi_timer(uint32_t timeout_ms)
{
  // xTimerChangePeriod will start timer if it's not running already
  if (xTimerChangePeriod(g_erp_context.isi_timer_handle,
      pdMS_TO_TICKS(timeout_ms), 0) == pdFAIL) {
    LOGE(TAG, "Unable to start pulse timer!");
  }
}

static void
stop_isi_timer(void)
{
  if (xTimerStop(g_erp_context.isi_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to stop pulse timer!");
  }
}

static void
erp_event_pulse_timeout(void)
{
  erp_event_t event = {.type = ERP_EVENT_PULSE_TIMEOUT};
  xQueueSend(g_event_queue, &event, 0);
}

static void
erp_event_isi_timeout(void)
{
  erp_event_t event = {.type = ERP_EVENT_ISI_TIMEOUT};
  xQueueSend(g_event_queue, &event, 0);
}

static void
pulse_timeout(TimerHandle_t timer_handle)
{
  erp_event_pulse_timeout();
}

static void
isi_timeout(TimerHandle_t timer_handle)
{
  erp_event_isi_timeout();
}


//
// Event handlers for the various application states:
//

static void
handle_state_standby(erp_event_t *event)
{
  switch (event->type) {
    case ERP_EVENT_ENTER_STATE:

      break;

    case ERP_EVENT_START:
      g_erp_context.erp_num_trials = event->data.erp.num_trials;
      g_erp_context.erp_pulse_dur_ms = event->data.erp.pulse_dur_ms;
      g_erp_context.erp_isi_ms = event->data.erp.isi_ms;
      g_erp_context.erp_jitter_ms = event->data.erp.jitter_ms;
      g_erp_context.erp_volume = event->data.erp.volume;
      set_state(ERP_STATE_RUNNING);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_running(erp_event_t *event) {
  switch (event->type) {
  case ERP_EVENT_ENTER_STATE:
  {
    g_erp_context.erp_count = 0;
    g_erp_context.sample_number = 0;

//    audio_set_volume(g_erp_context.erp_volume);
    audio_pink_default_volume();
    audio_pink_script_volume(g_erp_context.erp_volume/255.0f);
    audio_pink_computed_volume(1);
    audio_pink_mute(false);
    audio_pink_fadein(0);
    audio_pink_play();

    erp_event_isi_timeout();
    break;
  }
  case ERP_EVENT_ISI_TIMEOUT:
    audio_pink_mute(false);
    // log the pulse
    data_log_pulse(g_erp_context.sample_number, true);
    // wait for pulse duration
    restart_pulse_timer(g_erp_context.erp_pulse_dur_ms);
    break;

  case ERP_EVENT_PULSE_TIMEOUT:
  {
    // stop the pink noise
    audio_pink_mute(true);
    // log the pulse
    data_log_pulse(g_erp_context.sample_number, false);
    // increment the pulse counter
    g_erp_context.erp_count ++;
    if (g_erp_context.erp_count >= g_erp_context.erp_num_trials){
      erp_event_stop();
      break;
    }
    // compute isi
    uint32_t total_isi = g_erp_context.erp_isi_ms + ( rand() % (g_erp_context.erp_jitter_ms));
    restart_isi_timer( total_isi );
    break;
  }
  case ERP_EVENT_STOP:
    stop_pulse_timer();
    stop_isi_timer();

    // stop audio
    audio_pink_stop();
    audio_pink_fadeout(0);
    audio_pink_default_volume();

    // STOP THE ERP
    set_state(ERP_STATE_STANDBY);
    break;

  default:
    log_event_ignored(event);
    break;
  }

}

static void
handle_event(erp_event_t *event)
{
  switch (g_erp_context.state) {
    case ERP_STATE_STANDBY:
      handle_state_standby(event);
      break;

    case ERP_STATE_RUNNING:
      handle_state_running(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown erp_state: %d", (int) g_erp_context.state);
      break;
  }
}

void
erp_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(ERP_EVENT_QUEUE_SIZE,sizeof(erp_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "erp_event_queue");
}


static void
task_init()
{
  // Any post-scheduler init goes here.

  g_erp_context.pulse_timer_handle = xTimerCreateStatic("ERP_PULSE",
    pdMS_TO_TICKS(100), pdFALSE, NULL,
    pulse_timeout, &(g_erp_context.pulse_timer_struct));

  g_erp_context.isi_timer_handle = xTimerCreateStatic("ERP_ISI",
    pdMS_TO_TICKS(100), pdFALSE, NULL,
    isi_timeout, &(g_erp_context.isi_timer_struct));

  set_state(ERP_STATE_STANDBY);

  LOGV(TAG, "Task launched. Entering event loop.");
}


void
erp_task(void *ignored)
{

  task_init();

  erp_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

  }
}

#else (defined(ENABLE_ERP_TASK) && (ENABLE_ERP_TASK > 0U))

void erp_pretask_init(void){}
void erp_task(void *ignored){}

void erp_event_start(uint32_t num_trials, uint32_t pulse_dur_ms, uint32_t isi_ms, uint32_t jitter_ms, uint8_t volume){}
void erp_event_stop(){}
void erp_set_eeg_sample_number(unsigned long sample_number){}

#endif (defined(ENABLE_ERP_TASK) && (ENABLE_ERP_TASK > 0U))

