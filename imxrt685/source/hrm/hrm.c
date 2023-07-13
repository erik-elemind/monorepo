/*
 * Heart Rate Monitor Task
 *
 * Implements a periodic task. When turned on, it samples
 * from the heart rate monitor in the following order:
 *   BioSensor + Red LED
 *   BioSensor + IR LED
 *   Ambient Light Sensor + NO LED
 * The end of each sample is triggered by a pin interrupt.
 */
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "fsl_pint.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "ble.h"
#include "loglevels.h"
#include "config.h"
#include "peripherals.h"
#include "hrm.h"
#include "user_metrics.h"
#include "ml.h"

#if (defined(ENABLE_HRM_TASK) && (ENABLE_HRM_TASK > 0U))

#define HRM_EVENT_QUEUE_SIZE 10

static const char *TAG = "hrm";	// Logging prefix for this module

//
// Task events:
//
typedef enum
{
  HRM_EVENT_ENTER_STATE,
  HRM_EVENT_START,
  HRM_EVENT_ISR,
  HRM_EVENT_STOP,
} hrm_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  hrm_event_type_t type;
  void *user_data;
} hrm_event_t;


//
// State machine states:
//
typedef enum
{
  HRM_STATE_STANDBY,
  HRM_STATE_READ_FIFO,
} hrm_state_t;

//
// Global context data:
//
typedef struct
{
  hrm_state_t state;
} hrm_context_t;

static hrm_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[HRM_EVENT_QUEUE_SIZE*sizeof(hrm_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event( const hrm_event_t *event );

// For logging and debug:
static const char *
hrm_state_name(hrm_state_t state)
{
  switch (state) {
  	case HRM_STATE_STANDBY:     return "HRM_STATE_STANDBY";
    case HRM_STATE_READ_FIFO:  	return "HRM_STATE_READ_FIFO";
    default:
      break;
  }
  return "HRM_STATE UNKNOWN";
}


static const char *
hrm_event_type_name(hrm_event_type_t event_type)
{
  switch (event_type) {
    case HRM_EVENT_ENTER_STATE: return "HRM_EVENT_ENTER_STATE";
    case HRM_EVENT_START:      	return "HRM_EVENT_START";
    case HRM_EVENT_ISR:      	return "HRM_EVENT_ISR";
    case HRM_EVENT_STOP:     	return "HRM_EVENT_STOP";
    default:
      break;
  }
  return "HRM_EVENT UNKNOWN";
}

void
hrm_event_turn_off(void)
{
  static const hrm_event_t event = {.type = HRM_EVENT_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
hrm_event_turn_on(void)
{
  static const hrm_event_t event = {.type = HRM_EVENT_START };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
log_event(const hrm_event_t *event)
{
  switch (event->type) {
  case HRM_EVENT_ISR:
       // squelch this print
       break;
    default:
      LOGV(TAG, "[%s] Event: %s", hrm_state_name(g_context.state), hrm_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(const hrm_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s", hrm_state_name(g_context.state), hrm_event_type_name(event->type));
}

static void
set_state(hrm_state_t state)
{
  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  static const hrm_event_t event = { HRM_EVENT_ENTER_STATE };
  handle_event(&event);
}


//
// Event handlers for the various application states:
//
static void
handle_state_standby(const hrm_event_t *event)
{
	// Handle event triggered transition to the next state
	switch (event->type) {
		case HRM_EVENT_ENTER_STATE:
		break;

		case HRM_EVENT_START:
			max86140_start();
		break;

		case HRM_EVENT_ISR:
			set_state(HRM_STATE_READ_FIFO);
		break;

		case HRM_EVENT_STOP:
			max86140_stop();
		break;

		default:
			log_event_ignored(event);
		break;
	}
}

static void
handle_state_read_fifo(const hrm_event_t *event)
{
	// Handle event triggered transition to the next state
	switch (event->type) {
		case HRM_EVENT_ENTER_STATE:
		{
			uint8_t buff[MAX_FIFO_SAMPLES*3];
			uint8_t len = 0;
			max86140_process_fifo(buff, &len);

// Printing of the data, keep for now but eventually remove
//			printf("read %d samples\r\n", len);
//			for(uint8_t i=2;i<len;i+=3)
//			{
//				uint8_t tag = buff[i] >> 3;
//				int data = 0;
//				data = (buff[i+2] << 16) |  (buff[i+1] << 8) | (buff[i]);
//				data &= ~(0x1F << 18);
//
//				switch(tag)
//				{
//				case 0x01:
//				printf("LED1: ");
//				break;
//				case 0x02:
//				printf("LED2: ");
//				break;
//				case 0x03:
//				printf("LED3: ");
//				break;
//				case 0x04:
//				printf("Ambient: ");
//				break;
//				default:
//					printf("tag e %d\r\n", tag);
//					continue;
//				}
//
//				printf("0x%d\r\n", data);
//			}

			set_state(HRM_STATE_STANDBY);
		}
		break;

		case HRM_EVENT_ISR:
			LOGE(TAG, "Not processing ISR for HRM fast enough");
		break;

		case HRM_EVENT_STOP:
			max86140_stop();
			set_state(HRM_STATE_STANDBY);
		break;

		default:
			log_event_ignored(event);
		break;
	}
}


static void
handle_event(const hrm_event_t *event)
{
  switch (g_context.state) {

    case HRM_STATE_STANDBY:
      handle_state_standby(event);
      break;

    case HRM_STATE_READ_FIFO:
    	handle_state_read_fifo(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown hrm state: %d", (int) g_context.state);
      break;
  }
}


void
hrm_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(HRM_EVENT_QUEUE_SIZE,sizeof(hrm_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "hrm_event_queue");
}


static void
task_init()
{
  // Any post-scheduler init goes here.
  // Make sure HRM is off on boot up
  max86140_stop();

  // Initialize the state
  set_state(HRM_STATE_STANDBY);

  LOGV(TAG, "Task launched. Entering event loop.");
}

void
hrm_task(void *ignored)
{
  task_init();

  hrm_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

  } // while(1)
}


/*
 * Pin change interrupt, registered in the function call
 * "hrm_pretask_init()" and connected to heart rate monitor.
 *
 * The pin interrupt is associated with PINT output 0,
 * which has been associated with GPIO pin PIO0_16
 * in auto-generated pin_mux.h/c.
 */
void hrm_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // Send Event
  static const hrm_event_t event = {.type = HRM_EVENT_ISR, };
  xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);
  // Always do this when calling a FreeRTOS "...FromISR()" function:
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#else // (defined(ENABLE_HRM_TASK) && (ENABLE_HRM_TASK > 0U))

void hrm_pretask_init(void){}
void hrm_task(void *ignored){}
void hrm_event_turn_off(void){}
void hrm_event_turn_on(void){}

void hrm_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status){}

#endif // (defined(ENABLE_HRM_TASK) && (ENABLE_HRM_TASK > 0U))
