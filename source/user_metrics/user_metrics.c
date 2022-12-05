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

// Note: It looks like one "inference" takes a little less than 3 seconds.
// During inference the event queue is not being emptied.
// Since the event queue dominantly receives EEG sample events, which occur at 250Hz.
// 3 seconds * 250 Hz = 750 sample events.
// To provide buffer size, we're allocating a queue size of 1000 events.
#define USER_METRICS_EVENT_QUEUE_SIZE 10
static const char *TAG = "user_metrics";	// Logging prefix for this module

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
  int32_t eeg_fpz_sample;
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

void user_metrics_event_input(void)
{
    user_metrics_event_t event = {.type = USER_METRICS_EVENT_INPUT};
    //memcpy(&(event.eeg_fpz_sample), &(f_sample->eeg_channels[EEG_FPZ]), sizeof(f_sample->eeg_channels[EEG_FPZ]));
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
static void set_state(user_metrics_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]", user_metrics_state_name(g_context.state), user_metrics_state_name(state));

  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  user_metrics_event_t event = { USER_METRICS_EVENT_ENTER, (void *) state };
  handle_event(&event);
}

static void handle_state_standby(user_metrics_event_t *event)
{

  switch (event->type) {
    case USER_METRICS_EVENT_ENTER:
      // Generic code to always execute when entering this state goes here.
    	// reinit struct
      break;

    case USER_METRICS_EVENT_OPEN:
      set_state(USER_METRICS_STATE_OPEN);
      break;

    case USER_METRICS_EVENT_STOP:
    	user_metrics_log_close_command();
    	set_state(USER_METRICS_STATE_STANDBY);
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
      // Generic code to always execute when entering this state goes here.
      // Check timestamp
    	// fill in struct
    	// if struct full, save file
    	// set_state(USER_METRICS_STATE_STANDBY);
    	break;
    case USER_METRICS_EVENT_STOP:{
    	user_metrics_log_close_command();
    	set_state(USER_METRICS_STATE_STANDBY);
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
	  user_metrics_log_close_command();
	  user_metrics_log_open_command();
	  break;
  case USER_METRICS_EVENT_INPUT:
	  set_state(USER_METRICS_STATE_INPUT);
	  break;
  case USER_METRICS_EVENT_STOP:
	  user_metrics_log_close_command();
	  set_state(USER_METRICS_STATE_STANDBY);
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
  set_state(USER_METRICS_STATE_STANDBY);
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
