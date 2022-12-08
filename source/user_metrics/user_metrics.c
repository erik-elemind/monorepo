#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "loglevels.h"
#include "config.h"
#include "eeg_constants.h"
#include "user_metrics.h"
#include "data_log_commands.h"
#include "ff.h"

#define USER_METRICS_EVENT_QUEUE_SIZE 10
static const char *TAG = "user_metrics";	// Logging prefix for this module
static FIL user_metrics_log;
static int sample_count = 0;
//
// Task events:
//
typedef enum
{
  USER_METRICS_EVENT_ENTER,	// (used for state transitions)
  USER_METRICS_EVENT_OPEN,
  USER_METRICS_EVENT_INPUT,
  USER_METRICS_EVENT_STOP
} user_metrics_event_type_t;

// Events are passed to the  with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  user_metrics_event_type_t type;
  int data;
  user_metrics_data_t datatype;
} user_metrics_event_t;


//
// State machine states:
//
typedef enum
{
  USER_METRICS_STATE_STANDBY,
  USER_METRICS_STATE_INPUT,
  USER_METRICS_STATE_OPEN
} user_metrics_state_t;

//
// Global context data:
//
typedef struct
{
  user_metrics_state_t state;
  // TODO: Other "global" vars shared between events or states goes here.
} user_metrics_context_t;

static user_metrics_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[USER_METRICS_EVENT_QUEUE_SIZE*sizeof(user_metrics_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(user_metrics_event_t *event);

// For logging and debug:
static const char * user_metrics_state_name(user_metrics_state_t state)
{
  switch (state) {
    case USER_METRICS_STATE_STANDBY: return "USER_METRICS_STATE_STANDBY";
    case USER_METRICS_STATE_INPUT: return "USER_METRICS_STATE_INPUT";
    case USER_METRICS_STATE_OPEN: return "USER_METRICS_STATE_OPEN";
    default:
      break;
  }
  return "USER_METRICS_STATE UNKNOWN";
}

static const char * user_metrics_event_type_name(user_metrics_event_type_t event_type)
{
  switch (event_type) {
    case USER_METRICS_EVENT_ENTER: return "USER_METRICS_EVENT_ENTER";
    case USER_METRICS_EVENT_OPEN: return "USER_METRICS_EVENT_OPEN";
    case USER_METRICS_EVENT_INPUT: return "USER_METRICS_EVENT_INPUT";
    case USER_METRICS_EVENT_STOP: return "USER_METRICS_EVENT_STOP";
    default:
      break;
  }
  return "USER_METRICS_EVENT UNKNOWN";
}

void user_metrics_event_open(void)
{
    user_metrics_event_t event = {.type = USER_METRICS_EVENT_OPEN};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void user_metrics_event_input(int data, user_metrics_data_t datatype)
{
    user_metrics_event_t event = {.type = USER_METRICS_EVENT_INPUT, .datatype = datatype, .data=data};
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void user_metrics_event_stop(void)
{
	user_metrics_event_t event = {.type = USER_METRICS_EVENT_STOP};
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void log_event(user_metrics_event_t *event)
{
  switch (event->type) {
  case USER_METRICS_EVENT_INPUT:
	  break;
    default:
      LOGV(TAG, "[%s] Event: %s\n\r", user_metrics_state_name(g_context.state), user_metrics_event_type_name(event->type));
      break;
  }
}

static void log_event_ignored(user_metrics_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s\n\r", user_metrics_state_name(g_context.state), user_metrics_event_type_name(event->type));
}


//
// Event handlers for the various application states:
//
static void set_state(user_metrics_state_t state, user_metrics_event_t *cur_event)
{
  LOGD(TAG, "[%s] -> [%s]", user_metrics_state_name(g_context.state), user_metrics_state_name(state));

  g_context.state = state;

  // process first input
  if (cur_event->type == USER_METRICS_EVENT_INPUT)
  {
    user_metrics_event_t event = { .type = USER_METRICS_EVENT_ENTER, .data = cur_event->data};
    handle_event(&event);
  }
  else
  {
    // Immediately process an ENTER_STATE event, before any other pending events.
    // This allows the app to do state-specific init/setup when changing states.
    user_metrics_event_t event = { USER_METRICS_EVENT_ENTER, (void *) state };
    handle_event(&event);
  }
}

static void handle_state_standby(user_metrics_event_t *event)
{
  switch (event->type) {
    case USER_METRICS_EVENT_ENTER:
      // Generic code to always execute when entering this state goes here.
      break;
    case USER_METRICS_EVENT_INPUT:
      set_state(USER_METRICS_STATE_INPUT, event);
      break;

    case USER_METRICS_EVENT_OPEN:
      set_state(USER_METRICS_STATE_OPEN, event);
      break;

    case USER_METRICS_EVENT_STOP:
    	f_printf(&user_metrics_log, "]}");
    	f_sync(&user_metrics_log);
    	f_close(&user_metrics_log);
      sample_count = 0;
      set_state(USER_METRICS_STATE_STANDBY, event);
    	break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void handle_state_input(user_metrics_event_t *event)
{
  switch (event->type) {
    case USER_METRICS_EVENT_ENTER:
    case USER_METRICS_EVENT_INPUT:
    	sample_count++;
    	// dollar tree JSON
      if (sample_count == 1)
      {
    	  switch (event->datatype) {
			  case HYPNOGRAM_DATA:
				  f_printf(&user_metrics_log, "{\"ts\":%lu,\"rtss\":%d}", rtc_get(), event->data);
				  break;
			  case HRM_DATA:
				  f_printf(&user_metrics_log, "{\"ts\":%lu,\"bpm\":%d}", rtc_get(), event->data);
				  break;
			  case ACTIVITY_DATA:
				  f_printf(&user_metrics_log, "{\"ts\":%lu,\"act\":%d}", rtc_get(), event->data);
    		  break;
    	  }
      }
      else
      {
    	  switch (event->datatype) {
			  case HYPNOGRAM_DATA:
				  f_printf(&user_metrics_log, ",\n{\"ts\":%lu,\"rtss\":%d}", rtc_get(), event->data);
				  break;
			  case HRM_DATA:
				  f_printf(&user_metrics_log, ",\n{\"ts\":%lu,\"bpm\":%d}", rtc_get(), event->data);
				  break;
			  case ACTIVITY_DATA:
				  f_printf(&user_metrics_log, ",\n{\"ts\":%lu,\"act\":%d}", rtc_get(), event->data);
				  break;
    	  }
      }
      f_sync(&user_metrics_log);
      set_state(USER_METRICS_STATE_STANDBY, event);
      break;
    case USER_METRICS_EVENT_STOP:{
      f_printf(&user_metrics_log, "]}");
      f_sync(&user_metrics_log);
      f_close(&user_metrics_log);
      sample_count = 0;
      set_state(USER_METRICS_STATE_STANDBY, event);
      break;
    }

    default:
      log_event_ignored(event);
      break;
  }
}

static void handle_state_open(user_metrics_event_t *event)
{
  switch (event->type) {
  case USER_METRICS_EVENT_ENTER:
  case USER_METRICS_EVENT_OPEN:
    f_printf(&user_metrics_log, "]}");
    f_sync(&user_metrics_log);
    f_close(&user_metrics_log);
    sample_count = 0;
    user_metrics_log_open(&user_metrics_log);
    set_state(USER_METRICS_STATE_STANDBY, event);
    break;
  case USER_METRICS_EVENT_STOP:
    f_printf(&user_metrics_log, "]}");
    f_sync(&user_metrics_log);
    f_close(&user_metrics_log);
    sample_count = 0;
    set_state(USER_METRICS_STATE_STANDBY, event);
    break;
  default:
    log_event_ignored(event);
	  break;
  }
}

static void handle_event(user_metrics_event_t *event)
{
  switch (g_context.state) {
    case USER_METRICS_STATE_STANDBY:
      handle_state_standby(event);
      break;

    case USER_METRICS_STATE_INPUT:
      handle_state_input(event);
      break;

    case USER_METRICS_STATE_OPEN:
      handle_state_open(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown user_metrics state: %d\n\r", (int) g_context.state);
      break;
  }
}

void user_metrics_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(USER_METRICS_EVENT_QUEUE_SIZE,sizeof(user_metrics_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "user_metrics_event_queue");

}


static void task_init()
{
  // Any post-scheduler init goes here.
  set_state(USER_METRICS_STATE_STANDBY, NULL);
  LOGV(TAG, "Task launched. Entering event loop.\n\r");
}

void user_metrics_task(void *ignored)
{
  task_init();

  user_metrics_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);
  }
}
