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

#include "ml_utils.h"

// compile with iso c++17

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

#define EEG_BUF_SIZE 7500 	    	// 250 Hz  * 30 second epoch
#define ACCEL_BUF_SIZE 3750 		// 125 Hz  * 30 second epoch
#define HR_BUF_SIZE 3750  	    	// 125 Hz  * 30 second epoch

#define EEG_PREPROCESS_SIZE 7500 	// 250 Hz * 30 second epoch
#define ACCEL_PREPROCESS_SIZE 750 	// 25 Hz  * 30 second epoch
#define HR_PREPROCESS_SIZE 30  	    // 1 Hz   * 30 second epoch

#define ACCEL_HPF_ORDER 2
#define ACCEL_FS 25
#define ACCEL_FH 0.1

// Variables needed for double buffering method of ML data
static float g_eeg_A[EEG_BUF_SIZE];
static float g_eeg_B[EEG_BUF_SIZE];
static float* g_eeg_fill_p = g_eeg_A;
static float* g_eeg_ready_p = g_eeg_B;

static size_t g_eeg_fill_idx = 0;
static bool g_eeg_buf_ready = false;

static float g_accelx_A[ACCEL_BUF_SIZE];
static float g_accelx_B[ACCEL_BUF_SIZE];
static float g_accely_A[ACCEL_BUF_SIZE];
static float g_accely_B[ACCEL_BUF_SIZE];
static float g_accelz_A[ACCEL_BUF_SIZE];
static float g_accelz_B[ACCEL_BUF_SIZE];

// // Adds an extra 
// static float g_accelx_processed[ACCEL_BUF_SIZE]

static float* g_accelx_fill_p = g_accelx_A;
static float* g_accelx_ready_p = g_accelx_B;
static float* g_accely_fill_p = g_accely_A;
static float* g_accely_ready_p = g_accely_B;
static float* g_accelz_fill_p = g_accelz_A;
static float* g_accelz_ready_p = g_accelz_B;

static size_t g_accel_fill_idx = 0;
static bool g_accel_buf_ready = false;
static bool g_accel_process_done = false;
static bool g_accel_filt_en = false;

static float g_hr_A[HR_BUF_SIZE];
static float g_hr_B[HR_BUF_SIZE];
static float* g_hr_fill_p = g_hr_A;
static float* g_hr_ready_p = g_hr_B;

static size_t g_hr_fill_idx = 0;
static bool g_hr_buf_ready = false;
static bool g_hr_filt_en = false;

SemaphoreHandle_t g_sem = NULL;
StaticSemaphore_t g_ml_sem_buf;
#define ML_EVENT_QUEUE_SIZE 10

// Global memory
static const char *TAG = "ml";	// Logging prefix for this module

//
// Task events:
//
typedef enum
{
  ML_EVENT_ENTER,	// (used for state transitions)
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
  ML_STATE_PREPROCESS_DATA,
  ML_STATE_INFERENCE
} ml_state_t;

//
// Global context data:
//
typedef struct
{
  ml_state_t state;
  // TODO: Other "global" vars shared between events or states goes here.
  uint8_t ml_enabled;
  // move filter enables and such here too? pass the context around
} ml_context_t;

static ml_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[ML_EVENT_QUEUE_SIZE*sizeof(ml_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(ml_event_t *event);
static void set_state(ml_state_t state);
float output[OUTPUT_NUM_CLASS];

static accel_filter<FILT_TYPE, MAX_ACCEL_FILT_ORDER> g_accel_filt;

void filter_init(){
	// 2nd Order Butterworth filter. 0.1 Hz cutoff. Use the same filter for all dimensions
	g_accel_filt.designAccelFilter(ACCEL_HPF_ORDER, ACCEL_FS, ACCEL_FS, true); // reset cache by default
	g_accel_filt_en = true;
  	// g_hr_filt_en = true;
}

// For logging and debug:
static const char * ml_state_name(ml_state_t state)
{
  switch (state) {
    case ML_STATE_STANDBY: return "ML_STATE_STANDBY";
    case ML_STATE_PREPROCESS_DATA: return "ML_STATE_PREPROCESS_DATA";
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
    case ML_EVENT_STOP: return "ML_EVENT_STOP";
    default:
      break;
  }
  return "ML_EVENT UNKNOWN";
}

void ml_enable(void)
{
	g_context.ml_enabled = 1;
}

void ml_disable(void)
{
	g_context.ml_enabled = 0;
}

void ml_event_eeg_input(ads129x_frontal_sample* f_sample)
{
	// take pointer semaphore
	if ( xSemaphoreTake(g_sem, portMAX_DELAY) == pdTRUE )
	{
		if(!g_eeg_buf_ready)
		{
			g_eeg_fill_p[g_eeg_fill_idx] = f_sample->eeg_channels[EEG_FPZ]; // TODO: Implement smart channel switching
			g_eeg_fill_idx++;
			if(g_eeg_fill_idx == EEG_PREPROCESS_SIZE){
				g_eeg_fill_idx = 0;
				// flip double buffer
				if(g_eeg_fill_p == g_eeg_A)
				{
					g_eeg_fill_p = g_eeg_B;
					g_eeg_ready_p = g_eeg_A;
				}
				else
				{
					g_eeg_fill_p = g_eeg_A;
					g_eeg_ready_p = g_eeg_B;
				}
				memset(g_eeg_fill_p, 0, sizeof(g_eeg_fill_p));
				// update ready booleans
				g_eeg_buf_ready = true;
				// provide a synchronization point across all sensors
				if(g_eeg_buf_ready && g_accel_buf_ready && g_hr_buf_ready)
				{
					g_eeg_buf_ready = false;
					g_accel_buf_ready = false;
					g_hr_buf_ready = false;

					set_state(ML_STATE_PREPROCESS_DATA);
				}
			}
		}
		// give pointer semaphore
		xSemaphoreGive(g_sem);
	}
}

void ml_event_acc_input(lis2dtw12_sample_t* acc_sample)
{
	// take pointer semaphore
	if ( xSemaphoreTake(g_sem, portMAX_DELAY) == pdTRUE )
	{
		for(uint8_t i=0;i<32;i++)
		{
			if(!g_accel_buf_ready)
			{
				g_accelx_fill_p[g_accel_fill_idx] = (float) acc_sample[i].x;
				g_accely_fill_p[g_accel_fill_idx] = (float) acc_sample[i].y;
				g_accelz_fill_p[g_accel_fill_idx] = (float) acc_sample[i].z;

				g_accel_fill_idx++;

				if(g_accel_fill_idx == ACCEL_PREPROCESS_SIZE){
					g_accel_fill_idx = 0;
					// flip double buffer
					if(g_accelx_fill_p == g_accelx_A)
					{
						g_accelx_fill_p = g_accelx_B;
						g_accelx_ready_p = g_accelx_A;

						g_accely_fill_p = g_accely_B;
						g_accely_ready_p = g_accely_A;

						g_accelz_fill_p = g_accelz_B;
						g_accelz_ready_p = g_accelz_A;
					}
					else
					{
						g_accelx_fill_p = g_accelx_A;
						g_accelx_ready_p = g_accelx_B;

						g_accely_fill_p = g_accely_A;
						g_accely_ready_p = g_accely_B;

						g_accelz_fill_p = g_accelz_A;
						g_accelz_ready_p = g_accelz_B;
					}

					memset(g_accelx_fill_p, 0, sizeof(g_accelx_fill_p));
					memset(g_accely_fill_p, 0, sizeof(g_accely_fill_p));
					memset(g_accelz_fill_p, 0, sizeof(g_accelz_fill_p));
					// update ready booleans
					g_accel_buf_ready = true;


					LOGI(TAG, "preprocess the beb");
					set_state(ML_STATE_PREPROCESS_DATA);

					// provide a synchronization point across all sensors
					// if(g_eeg_buf_ready && g_accel_buf_ready && g_hr_buf_ready)
					// {
					// 	g_eeg_buf_ready = false;
					// 	g_accel_buf_ready = false;
					// 	g_hr_buf_ready = false;

					// 	set_state(ML_STATE_PREPROCESS_DATA);
					// }
				}
			}
			else
			{
				break;
			}
		}
		// give pointer semaphore
		xSemaphoreGive(g_sem);
	}
}

void ml_event_hr_input(uint8_t hr_sample)
{
	// take pointer semaphore
	if ( xSemaphoreTake(g_sem, portMAX_DELAY) == pdTRUE )
	{
		if(!g_hr_buf_ready)
		{
			g_hr_fill_p[g_hr_fill_idx] = (float) hr_sample;
			g_hr_fill_idx++;
			if(g_hr_fill_idx == HR_PREPROCESS_SIZE){
				g_hr_fill_idx = 0;
				// flip double buffer
				if(g_hr_fill_p == g_hr_A)
				{
					g_hr_fill_p = g_hr_B;
					g_hr_ready_p = g_hr_A;
				}
				else
				{
					g_hr_fill_p = g_hr_A;
					g_hr_ready_p = g_hr_B;
				}
				memset(g_hr_fill_p, 0, sizeof(g_hr_fill_p));
				// update ready booleans
				g_hr_buf_ready = true;
				// provide a synchronization point across all sensors
				if(g_eeg_buf_ready && g_accel_buf_ready && g_hr_buf_ready)
				{
					g_eeg_buf_ready = false;
					g_accel_buf_ready = false;
					g_hr_buf_ready = false;

					set_state(ML_STATE_PREPROCESS_DATA);
				}
			}
		}
		// give pointer semaphore
		xSemaphoreGive(g_sem);
	}
}

void ml_event_stop(void)
{
	ml_event_t event = {.type = ML_EVENT_STOP};
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void log_event(ml_event_t *event)
{
  switch (event->type) {
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
  LOGD(TAG, "[%s] -> [%s]", ml_state_name(g_context.state), ml_state_name(state));

  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  ml_event_t event = { ML_EVENT_ENTER };
  handle_event(&event);

}

static void handle_state_standby(ml_event_t *event)
{

  switch (event->type) {
    case ML_EVENT_ENTER:
      // Generic code to always execute when entering this state goes here.
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

// Should we include events for configuration like in eeg_processor.cpp?
// Depends on how often we plan to live reconfigure the preprocessing pipeline 
static void handle_state_preprocess_data(ml_event_t *event)
{
  switch (event->type) {
    case ML_EVENT_ENTER:{
      // Todo: Implement pre-processing of all data for inference
		if (g_accel_filt_en && g_accel_buf_ready) {
			LOGI(TAG, "enter the beb");
			g_accel_process_done = false;

			// 2nd order Butterworth Filter
			for (int i; i < ACCEL_BUF_SIZE; i++){ 
				g_accelx_ready_p[i] = g_accel_filt.filter(g_accelx_ready_p[i]); // feed values in one at a time
				// g_accely_ready_p[i] = g_accel_filt.filter(g_accely_ready_p[i]); // check eeg filt again to see how david advances the buffer
				// g_accelz_ready_p[i] = g_accel_filt.filter(g_accelz_ready_p[i]);
			}

			// Upsampling to 125 Hz -> make these defines
			int fs_orig = 25; // Hz
			int fs_new = 125; // Hz
			
			// Cast to vector to resample
			std::vector<float, bufferAllocator<float>> accelx_vec(g_accelx_ready_p, g_accelx_ready_p + ACCEL_BUF_SIZE);
			// std::vector<float, bufferAllocator<float>> accely_vec(g_accely_ready_p, g_accely_ready_p + ACCEL_BUF_SIZE);
			// std::vector<float, bufferAllocator<float>> accelz_vec(g_accelz_ready_p, g_accelz_ready_p + ACCEL_BUF_SIZE);

			// std::vector<float, bufferAllocator<float>> accelx_vec_out;
			// std::vector<float, bufferAllocator<float>> accely_vec_out;
			// std::vector<float, bufferAllocator<float>> accelz_vec_out;

			resample<float> (fs_orig, fs_new, accelx_vec, accelx_vec);
			// resample<float> (fs_orig, fs_new, accely_vec, accely_vec);
			// resample<float> (fs_orig, fs_new, accelz_vec, accelz_vec);

			accelx_vec = z_score_normalize(accelx_vec);
			// accely_vec = z_score_normalize(accely_vec);
			// accelz_vec = z_score_normalize(accelz_vec);

			g_accel_process_done = true;

			LOGI(TAG, "processed the beb");
			// send semaphore to inference task that buffer is ready
		} 

    	// (g_context.ml_enabled > 0) ? set_state(ML_STATE_INFERENCE) : set_state(ML_STATE_STANDBY);
		// Assume no race condition since we have the whole double buffer thing anyway?

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
		case ML_EVENT_ENTER:{
			// Generic code to always execute when entering this state goes here.

			// Load up sample input data
			memcpy(bundleInpAddr, ((char *)TEST_MODEL_INPUT), sizeof(TEST_MODEL_INPUT));

			// Run model inference
			int rc = test_model(constantWeight, mutableWeight, activations);

			if(rc != 0)
			{
				LOGE(TAG, "Inference failed: %d", rc);
				set_state(ML_STATE_STANDBY);
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

			set_state(ML_STATE_STANDBY);
			break;
		}
		default:
			log_event_ignored(event);
			break;
  }
}

static void handle_event(ml_event_t *event)
{

  switch (event->type) {
    case ML_EVENT_STOP:
      ml_disable();

      g_eeg_fill_p = g_eeg_A; // the alloc mem buf is moving into this memory and fucking things up
      g_eeg_ready_p = g_eeg_B;

      g_eeg_fill_idx = 0;
      g_eeg_buf_ready = false;

      g_accelx_fill_p = g_accelx_A;
      g_accelx_ready_p = g_accelx_B;
      g_accely_fill_p = g_accely_A;
      g_accely_ready_p = g_accely_B;
      g_accelz_fill_p = g_accelz_A;
      g_accelz_ready_p = g_accelz_B;

      g_accel_fill_idx = 0;
      g_accel_buf_ready = false;

      g_hr_fill_p = g_hr_A;
      g_hr_ready_p = g_hr_B;

      g_hr_fill_idx = 0;
      g_hr_buf_ready = false;
      return;

    default:
      break;
  }

  switch (g_context.state) {
    case ML_STATE_STANDBY:
      handle_state_standby(event);
      break;
    case ML_STATE_PREPROCESS_DATA:
      handle_state_preprocess_data(event);
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

  // Initialize custom allocator
  mm_rtos_init(&alloc_mem, alloc_mem_buf, ALLOC_MEM*sizeof(float));

  // Load up constant weights for ML model
  memcpy(constantWeight, WEIGHT_DATA_START, TEST_MODEL_CONSTANT_MEM_SIZE);

//   g_sem = xSemaphoreCreateBinary();
  g_sem = xSemaphoreCreateBinaryStatic(&g_ml_sem_buf);

  if (g_sem == NULL)
  {
	/* There was not enough heap memory space to create the semaphore. Handle this case here. */
	LOGE(TAG, "Could not create semaphore");
  }
  else
  {
	  xSemaphoreGive(g_sem);
  }

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(ML_EVENT_QUEUE_SIZE,sizeof(ml_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "ml_event_queue");
  filter_init();

}


static void task_init()
{
  // Any post-scheduler init goes here.
  ml_disable();
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
