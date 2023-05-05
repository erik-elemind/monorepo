#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "loglevels.h"
#include "config.h"
#include "eeg_constants.h"
#include "ml.h"
#include "data_log_commands.h"
#include "user_metrics.h"

///>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> TEMP Placeholder for final model implementation
#include "sample_input.h" // For testing, will remove at some point

// Model input data FLASH section
extern const char input_section[];
#define INPUT_DATA_START  ((void *)input_section)
// Model weights FLASH section
extern const char weights_section[];
#define WEIGHT_DATA_START ((void *)weights_section)
// Model reference data FLASH section
extern const char output_section[];
#define OUTPUT_DATA_START ((void **)output_section)

// Statically allocate memory for constant weights.
GLOW_MEM_ALIGN(TEST_MODEL_MEM_ALIGN)
uint8_t constantWeight[TEST_MODEL_CONSTANT_MEM_SIZE];

// Statically allocate memory for mutable weights (model input/output data).
GLOW_MEM_ALIGN(TEST_MODEL_MEM_ALIGN)
uint8_t mutableWeight[TEST_MODEL_MUTABLE_MEM_SIZE];

// Statically allocate memory for activations (model intermediate results).
GLOW_MEM_ALIGN(TEST_MODEL_MEM_ALIGN)
uint8_t activations[TEST_MODEL_ACTIVATIONS_MEM_SIZE];
//
//// Bundle input/output data absolute addresses.
uint8_t *bundleInpAddr = GLOW_GET_ADDR(mutableWeight, TEST_MODEL_serving_default_input_0);
uint8_t *bundleOutAddr = GLOW_GET_ADDR(mutableWeight, TEST_MODEL_StatefulPartitionedCall_0);

// Model number of output classes.
#define OUTPUT_NUM_CLASS 5
///>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


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
  ML_EVENT_ENTER,	// (used for state transitions)
  ML_EVENT_INPUT,
  ML_EVENT_STOP
} ml_event_type_t;

// Events are passed to the  with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  ml_event_type_t type;
  int32_t eeg_fpz_sample;
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
  uint8_t enabled;
} ml_context_t;

static ml_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[ML_EVENT_QUEUE_SIZE*sizeof(ml_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(ml_event_t *event);

float output[OUTPUT_NUM_CLASS];
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
    case ML_EVENT_ENTER: return "ML_EVENT_ENTER";
    case ML_EVENT_INPUT: return "ML_EVENT_INPUT";
    case ML_EVENT_STOP: return "ML_EVENT_STOP";
    default:
      break;
  }
  return "ML_EVENT UNKNOWN";
}

void ml_enable(void)
{
	g_context.enabled = 1;
}

void ml_disable(void)
{
	g_context.enabled = 0;
}

void ml_event_eeg_input(ads129x_frontal_sample* f_sample)
{
  if (f_sample->eeg_sample_number % 2 == 0) // ML model expects downsampled (1/2) input
  {
    ml_event_t event = {.type = ML_EVENT_INPUT};
    memcpy(&(event.eeg_fpz_sample), &(f_sample->eeg_channels[EEG_FPZ]), sizeof(f_sample->eeg_channels[EEG_FPZ]));
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void ml_event_hr_input(uint8_t hr_sample)
{

}

void ml_event_acc_input(lis2dtw12_sample_t acc_sample)
{

}

void ml_event_stop(void)
{
	ml_event_t event = {.type = ML_EVENT_STOP};
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

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
static void set_state(ml_state_t state, ml_event_t *cur_event)
{
  LOGD(TAG, "[%s] -> [%s]", ml_state_name(g_context.state), ml_state_name(state));

  g_context.state = state;

  // process the first input received
  if (cur_event->type == ML_EVENT_INPUT)
  {
    ml_event_t event = { .type = ML_EVENT_ENTER, .eeg_fpz_sample = cur_event->eeg_fpz_sample};
    handle_event(&event);
  }
  else
  {
    // Immediately process an ENTER_STATE event, before any other pending events.
    // This allows the app to do state-specific init/setup when changing states.
    ml_event_t event = { ML_EVENT_ENTER, (void *) state };
    handle_event(&event);
  }
}

static void handle_state_standby(ml_event_t *event)
{

  switch (event->type) {
    case ML_EVENT_ENTER:
      // Generic code to always execute when entering this state goes here.
      break;

    case ML_EVENT_INPUT:
      set_state(ML_STATE_INPUT, event);
      break;

    case ML_EVENT_STOP:
      ml_disable();
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
    case ML_EVENT_ENTER:
    	currentCount = 0;
    case ML_EVENT_INPUT:{
      // Generic code to always execute when entering this state goes here.
      // Convert and store raw data to model input buffer (in V)
      
      model_input[currentCount] = (float) event->eeg_fpz_sample * EEG_SCALAR_V;
      currentCount++;

    	if (currentCount == INPUT_SIZE) // raw data is downsampled by half
    	{
    		(g_context.enabled > 0) ? set_state(ML_STATE_INFERENCE, event) : set_state(ML_STATE_STANDBY, event);
    		currentCount = 0;
    	}

      break;
    }
    case ML_EVENT_STOP:{
    	set_state(ML_STATE_STANDBY, event);
    	currentCount = 0;
    	ml_disable();
    	break;
    }

    default:
      log_event_ignored(event);
      break;
  }
}

static void handle_state_inference(ml_event_t *event)
{
	float max_val = 0.0;
	uint16_t max_idx = 0;

	switch (event->type) {
		case ML_EVENT_ENTER:
			// Generic code to always execute when entering this state goes here.

			// Load up sample input data
			memcpy(bundleInpAddr, ((char *)TEST_MODEL_INPUT), sizeof(TEST_MODEL_INPUT));

			// Run model inference
			int rc = test_model(constantWeight, mutableWeight, activations);

			if(rc != 0)
			{
				LOGE(TAG, "Inference failed: %d", rc);
				set_state(ML_STATE_STANDBY, event);
				return;
			}

			float *out_data  = (float *)(bundleOutAddr);


			// choose highest probability as predicted class
			for (int i = 0; i < OUTPUT_NUM_CLASS; i++)
			{
				if (out_data[i] > max_val)
				{
					max_val = out_data[i];
					max_idx = i;
				}
			}

			LOGV(TAG, "Inference output: %f, %f, %f, %f, %f\n\r", out_data[0], out_data[1], out_data[2], out_data[3], out_data[4]);
			LOGV(TAG, "Prediction: %d", max_idx);

			user_metrics_event_input(max_idx, HYPNOGRAM_DATA);

			set_state(ML_STATE_STANDBY, event);
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

  // Load up constant weights for ML model
  memcpy(constantWeight, WEIGHT_DATA_START, TEST_MODEL_CONSTANT_MEM_SIZE);

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(ML_EVENT_QUEUE_SIZE,sizeof(ml_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "ml_event_queue");

}


static void task_init()
{
  // Any post-scheduler init goes here.
  ml_disable();
  set_state(ML_STATE_STANDBY, NULL);
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
