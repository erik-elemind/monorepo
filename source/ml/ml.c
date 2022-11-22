#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "loglevels.h"
#include "config.h"
#include "ml.h"

#include "rt_nonfinite.h"
#include "sleepstagescorer.h"
#include "sleepstagescorer_terminate.h"

// Note: It looks like one "inference" takes a little less than 3 seconds.
// During inference the event queue is not being emptied.
// Since the event queue dominantly receives EEG sample events, which occur at 250Hz.
// 3 seconds * 250 Hz = 750 sample events.
// To provide buffer size, we're allocating a queue size of 1000 events.
#define ML_EVENT_QUEUE_SIZE 1000
#define INPUT_SIZE 1250

static const char *TAG = "ml";	// Logging prefix for this module

//
// Task events:
//
typedef enum
{
  ML_EVENT_ENTER_STATE,	// (used for state transitions)
  ML_EVENT_INPUT
} ml_event_type_t;

// Events are passed to the  with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  ml_event_type_t type;
  ads129x_frontal_sample eeg_sample;
} ml_event_t;


//
// State machine states:
//
typedef enum
{
  ML_STATE_STANDBY,
  ML_STATE_INPUT,
  ML_STATE_INFERENCE
} ml_state_t;

//
// Global context data:
//
typedef struct
{
  ml_state_t state;
  // TODO: Other "global" vars shared between events or states goes here.
} ml_context_t;

static ml_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[ML_EVENT_QUEUE_SIZE*sizeof(ml_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(ml_event_t *event);
float output[5];
float model_input[INPUT_SIZE];

// For logging and debug:
static const char * ml_state_name(ml_state_t state)
{
  switch (state) {
    case ML_STATE_STANDBY: return "ML_STATE_STANDBY";
    case ML_STATE_INPUT: return "ML_STATE_INPUT";
    case ML_STATE_INFERENCE: return "ML_STATE_INFERENCE";
    default:
      break;
  }
  return "ML_STATE UNKNOWN";
}

static const char * ml_event_type_name(ml_event_type_t event_type)
{
  switch (event_type) {
    case ML_EVENT_ENTER_STATE: return "ML_EVENT_ENTER_STATE";
    case ML_EVENT_INPUT: return "ML_EVENT_INPUT";
    default:
      break;
  }
  return "ML_EVENT UNKNOWN";
}

void ml_event_input(ads129x_frontal_sample* f_sample)
{
  ml_event_t event = {.type = ML_EVENT_INPUT};
  memcpy(&(event.eeg_sample), f_sample, sizeof(event.eeg_sample));
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

//void ml_event_output(int *output) // on output, send output to application or something to eventually log
//{
////  dils_event_t event = {.type = DILS_EVENT_OUTPUT, .user_data = NULL };
////  xQueueSend(g_event_queue, &event, portMAX_DELAY);
//	LOGV(TAG, "Output: %d", output);
//}

static void log_event(ml_event_t *event)
{
  switch (event->type) {
  case ML_EVENT_INPUT:
	  break;
    default:
      LOGV(TAG, "[%s] Event: %s\n\r", ml_state_name(g_context.state), ml_event_type_name(event->type));
      break;
  }
}

static void log_event_ignored(ml_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s\n\r", ml_state_name(g_context.state), ml_event_type_name(event->type));
}


//
// Event handlers for the various application states:
//
static void set_state(ml_state_t state)
{
  //LOGD(TAG, "[%s] -> [%s]", ml_state_name(g_context.state), ml_state_name(state));

  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  ml_event_t event = { ML_EVENT_ENTER_STATE, (void *) state };
  handle_event(&event);
}

static void handle_state_standby(ml_event_t *event)
{

  switch (event->type) {
    case ML_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      break;

    case ML_EVENT_INPUT:
      set_state(ML_STATE_INPUT);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void handle_state_input(ml_event_t *event)
{

  static int currentCount = 0;
  switch (event->type) {
    case ML_EVENT_INPUT:{
      // Generic code to always execute when entering this state goes here.
    	// store event data to model_input buffer
    	ads129x_frontal_sample *eeg_sample = &(event->eeg_sample);
    	if ((eeg_sample->eeg_sample_number % 2 == 0) && (currentCount < INPUT_SIZE))
    	{
    		// TODO: This mem copy from int32 to float looks suspicious.
    		memcpy(&model_input[currentCount], &(eeg_sample->eeg_channels[EEG_FPZ]), sizeof(eeg_sample->eeg_channels[EEG_FPZ]) );
    		currentCount++;
    	}

    	if (currentCount == INPUT_SIZE) // raw data is downsampled by half
    	{
    		set_state(ML_STATE_INFERENCE);
    		currentCount = 0;
    	}
      break;
    }

    default:
      log_event_ignored(event);
      break;
  }
}

static void handle_state_inference(ml_event_t *event)
{
  switch (event->type) {
    case ML_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.

      // hifi_inference(constantWeight, mutableWeight, activations);
      sleepstagescorer(model_input, output);
      LOGV(TAG, "Inference output: %f, %f, %f, %f, %f\n\r", output[0], output[1], output[2], output[3], output[4]);
      set_state(ML_STATE_STANDBY);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void handle_event(ml_event_t *event)
{
  switch (g_context.state) {
    case ML_STATE_STANDBY:
      handle_state_standby(event);
      break;

    case ML_STATE_INPUT:
      handle_state_input(event);
      break;

    case ML_STATE_INFERENCE:
      handle_state_inference(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown ml state: %d\n\r", (int) g_context.state);
      break;
  }
}

void ml_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(ML_EVENT_QUEUE_SIZE,sizeof(ml_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "ml_event_queue");

}


static void task_init()
{
  // Any post-scheduler init goes here.
  set_state(ML_STATE_STANDBY);
  LOGV(TAG, "Task launched. Entering event loop.\n\r");
}

void ml_task(void *ignored)
{
  task_init();

  ml_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);
  }
}
