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
  HRM_EVENT_TURN_ON,
  HRM_EVENT_START_SAMPLE,
  HRM_EVENT_ISR_DONE,
  HRM_EVENT_TURN_OFF,
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
  HRM_STATE_OFF,
  HRM_STATE_STANDBY,
  HRM_STATE_READ_RED,
  HRM_STATE_READ_IR,
  HRM_STATE_READ_ALS,
  HRM_STATE_PROCESS,
} hrm_state_t;

//
// Global context data:
//
typedef struct
{
  hrm_state_t state;
  // The previous time the hrm was sampled
  TickType_t prev_sample_complete_ticks;
  // The period at which the HRM should be sampled
  TickType_t sample_period_ticks;
  // Timer to periodically start red->ir->als->process.
  TimerHandle_t sample_timer;
  StaticTimer_t sample_timer_struct;
  // Most recent RED reflectivity sample.
  uint16_t red;
  // Most recent IR reflectivity sample.
  uint16_t ir;
  // Most recent Ambient Light Sensor reflectivity sample.
  uint16_t als;
} hrm_context_t;

static hrm_context_t g_context;
static vcnl4020c vcnl;
static heart_rate_monitor hrm;

// Global event queue and handler:
static uint8_t g_event_queue_array[HRM_EVENT_QUEUE_SIZE*sizeof(hrm_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event( const hrm_event_t *event );
void hrm_sample_timer_cb( TimerHandle_t xTimer );

// For logging and debug:
static const char *
hrm_state_name(hrm_state_t state)
{
  switch (state) {
    case HRM_STATE_OFF:      return "HRM_STATE_OFF";
    case HRM_STATE_STANDBY:  return "HRM_STATE_STANDBY";
    case HRM_STATE_READ_RED: return "HRM_STATE_READ_RED";
    case HRM_STATE_READ_IR:  return "HRM_STATE_READ_IR";
    case HRM_STATE_READ_ALS: return "HRM_STATE_READ_ALS";
    case HRM_STATE_PROCESS:  return "HRM_STATE_PROCESS";
    default:
      break;
  }
  return "HRM_STATE UNKNOWN";
}


static const char *
hrm_event_type_name(hrm_event_type_t event_type)
{
  switch (event_type) {
    case HRM_EVENT_ENTER_STATE:  return "HRM_EVENT_ENTER_STATE";
    case HRM_EVENT_TURN_ON:      return "HRM_EVENT_TURN_ON";
    case HRM_EVENT_START_SAMPLE: return "HRM_EVENT_START_SAMPLE";
    case HRM_EVENT_ISR_DONE:     return "HRM_EVENT_ISR_DONE";
    case HRM_EVENT_TURN_OFF:     return "HRM_EVENT_TURN_OFF";
    default:
      break;
  }
  return "HRM_EVENT UNKNOWN";
}

void
hrm_event_turn_off(void)
{
  static const hrm_event_t event = {.type = HRM_EVENT_TURN_OFF };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
hrm_event_turn_on(void)
{
  static const hrm_event_t event = {.type = HRM_EVENT_TURN_ON };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
log_event(const hrm_event_t *event)
{
  switch (event->type) {
    default:
      //LOGV(TAG, "[%s] Event: %s", hrm_state_name(g_context.state), hrm_event_type_name(event->type));
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
  //LOGD(TAG, "[%s] -> [%s]", hrm_state_name(g_context.state), hrm_state_name(state));

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
handle_state_off(const hrm_event_t *event)
{
  // Handle event triggered transition to the next state
  switch (event->type) {
    case HRM_EVENT_ENTER_STATE:
      // Stop sampling timer
      if( xTimerStop( g_context.sample_timer, portMAX_DELAY ) != pdPASS )
      {
        // Sampling timer failed to stop
        LOGE(TAG, "Failed to stop sampling timer.");
      }
    break;

    case HRM_EVENT_TURN_ON:
      // Zero out previous set of sampled values.
      g_context.red = 0;
      g_context.ir = 0;
      g_context.als = 0;
      // Re-initialize the HRM
      hrm_reinit(&hrm);
      // Immediate transition to the next state
      set_state(HRM_STATE_STANDBY);
      // Start sampling timer
      // Note: The sample timer must be started after
      //       the state has been changed to "standby".
      if( xTimerStart( g_context.sample_timer, portMAX_DELAY ) != pdPASS )
      {
        // Sampling timer failed to start
        LOGE(TAG, "Failed to start sampling timer.");
      }
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_standby(const hrm_event_t *event)
{
  // Handle event triggered transition to the next state
  switch (event->type) {
    case HRM_EVENT_ENTER_STATE:
      break;

    case HRM_EVENT_START_SAMPLE:
      set_state(HRM_STATE_READ_RED);
      break;

    case HRM_EVENT_TURN_OFF:
      set_state(HRM_STATE_OFF);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_red(const hrm_event_t *event)
{
  // Data for interrupts
  hrm_interrupt_status_val intstat;

  switch (event->type) {
    case HRM_EVENT_ENTER_STATE:
      // READ RED LED
      // Measured Timing with I2C @ 100,000 baud
      // reading_time + i2c_time = total_time
      // 300us          2.5ms      2.8ms
      hrm_sample_red(&hrm);
      break;

    case HRM_EVENT_ISR_DONE:
      // check interrupt and save off value
      intstat = vcnl4020c_get_interrupt_status(hrm.sensor);
      if(intstat.int_bs_ready){
        g_context.red = vcnl4020c_get_biosensor(hrm.sensor);
        intstat.int_bs_ready=false;
        vcnl4020c_clr_interrupt_status(hrm.sensor);
        set_state(HRM_STATE_READ_IR);
      }
      break;

    case HRM_EVENT_TURN_OFF:
      set_state(HRM_STATE_OFF);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_ir(const hrm_event_t *event)
{
  // Data for interrupts
  hrm_interrupt_status_val intstat;

  switch (event->type) {
    case HRM_EVENT_ENTER_STATE:
      // READ IR LED
      // Measured Timing with I2C @ 100,000 baud
      // reading_time + i2c_time = total_time
      // 300us          2.5ms      2.8ms
      hrm_sample_ir(&hrm);
      break;

    case HRM_EVENT_ISR_DONE:
      // check interrupt and save off value
      intstat = vcnl4020c_get_interrupt_status(hrm.sensor);
      if(intstat.int_bs_ready){
        g_context.ir = vcnl4020c_get_biosensor(hrm.sensor);
        intstat.int_bs_ready=false;
        vcnl4020c_clr_interrupt_status(hrm.sensor);
        set_state(HRM_STATE_READ_ALS);
      }
      break;

    case HRM_EVENT_TURN_OFF:
      set_state(HRM_STATE_OFF);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_als(const hrm_event_t *event)
{
  // Data for interrupts
  hrm_interrupt_status_val intstat;

  switch (event->type) {
    case HRM_EVENT_ENTER_STATE:
      // READ ALS LED
      // Measured Timing with I2C @ 100,000 baud
      // #conv,  reading_time + i2c_time =  total_time
      // 4       2.95ms         2.48ms      5.43ms
      // 8       5.5ms          2.55ms      8.05ms
      // 16      10.8ms         2.5 ms      13.3ms
      // 32      21.2ms         2.6 ms      23.8ms
      hrm_sample_ambient(&hrm);
      break;

    case HRM_EVENT_ISR_DONE:
      // check interrupt and save off value
      intstat = vcnl4020c_get_interrupt_status(hrm.sensor);
      if(intstat.int_als_ready){
        g_context.als = vcnl4020c_get_ambient(hrm.sensor);
        intstat.int_als_ready=false;
        vcnl4020c_clr_interrupt_status(hrm.sensor);
        set_state(HRM_STATE_PROCESS);
      }
      else{
        // other interrupt occurred. clear it and wait for ours.
        vcnl4020c_clr_interrupt_status(hrm.sensor);
      }
      break;

    case HRM_EVENT_TURN_OFF:
      set_state(HRM_STATE_OFF);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_process(const hrm_event_t *event)
{
	static int count = 0;
	static int count_ml = 0;

  switch (event->type) {
    case HRM_EVENT_ENTER_STATE:
      // Process the Data
      // Get the previous time the hrm sampling completed.
      g_context.prev_sample_complete_ticks = xTaskGetTickCount();

      hrm_update(&hrm, g_context.red, g_context.ir);

      //LOGV(TAG, "lux %%  : red %f, ir %f, als %f", g_context.red/65535.0, g_context.ir/65535.0, g_context.als/65535.0);
      //LOGV(TAG, "current: Ired %dmA, Iir %dmA", hrm.red_curr_regval*10, hrm.ir_curr_regval*10 );
      //ble_heart_rate_update(round(hrm_get_heart_rate(&hrm)));

      //TODO: placeholder until bpm calculation figured out
      count++;
      if (count == 100) // 100 samples at 10Hz is 10 seconds
      {
    	  user_metrics_event_input((rand() % 35)+50, HRM_DATA);
    	  count = 0;
      }

      count_ml++;
      if(count_ml == 10)
      {
    	  count_ml=0;
    	  ml_event_hr_input((rand() % 35)+50);
      }
        // Wait for the timer to start the next sample
        set_state(HRM_STATE_STANDBY);
      break;

    case HRM_EVENT_TURN_OFF:
      set_state(HRM_STATE_OFF);
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
    case HRM_STATE_OFF:
      handle_state_off(event);
      break;

    case HRM_STATE_STANDBY:
      handle_state_standby(event);
      break;

    case HRM_STATE_READ_RED:
      handle_state_red(event);
      break;

    case HRM_STATE_READ_IR:
      handle_state_ir(event);
      break;

    case HRM_STATE_READ_ALS:
      handle_state_als(event);
      break;

    case HRM_STATE_PROCESS:
      handle_state_process(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown led state: %d", (int) g_context.state);
      break;
  }
}


void
hrm_pretask_init(void)
{
  // Any pre-scheduler init goes here.
  vcnl4020c_init(&vcnl, &HRM_I2C_RTOS_HANDLE,
      HRM_RED_ENABLE_PORT, HRM_RED_ENABLE_PIN,
      HRM_IR_ENABLE_PORT, HRM_IR_ENABLE_PIN,
      HRM_PINT_PIN, hrm_pint_isr, true);

  hrm_init(&hrm, &vcnl, 0.5f, 0.1f, 0.0027f);

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(HRM_EVENT_QUEUE_SIZE,sizeof(hrm_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "hrm_event_queue");
}


static void
task_init()
{
  // Any post-scheduler init goes here.

  // Set sample period
  // 50  ms = 20Hz
  // 100 ms = 10Hz
  g_context.sample_period_ticks = pdMS_TO_TICKS(100);

  g_context.sample_timer = xTimerCreateStatic(
      "HRM_Sample_Timer",
      g_context.sample_period_ticks,
      pdTRUE,
      ( void * ) 0,
      hrm_sample_timer_cb,
      &(g_context.sample_timer_struct));

  //    hrm_product_id_val product_id = vcnl4020c_get_product_id(hrm.sensor);
  //    LOGV(TAG, " %d %d \n",product_id.product_id,product_id.revision_id);

  // Ambient Sensor sample time in continuous conversion mode:
  // general equations
  // sample_time_ms = 0.460 + 0.460 * (2^samples_to_average)
  // samples_to_average  # conv  sample_time_ms
  // 0                   1       1    ms
  // 1                   2       1.4  ms
  // 2                   4       2.3  ms
  // 3                   8       4.2  ms
  // 4                   16      7.82 ms
  // 5                   32      15.18ms
  // 6                   64      29.9 ms
  // 7                   128     60   ms
  uint8_t samples_to_average = 2;
  vcnl4020c_set_ambient_config(&vcnl, true, 1, true, samples_to_average);

  // Initialize the state
  set_state(HRM_STATE_OFF);

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
  static const hrm_event_t event = {.type = HRM_EVENT_ISR_DONE, };
  xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);
  // Always do this when calling a FreeRTOS "...FromISR()" function:
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*
 * Called periodically from an xTimer to kick off one red, ir, als sample.
 */
void hrm_sample_timer_cb( TimerHandle_t xTimer )
{
  static const hrm_event_t event = {.type = HRM_EVENT_START_SAMPLE, };
  // Do wait forever here due to the repeated nature of this event.
  xQueueSend(g_event_queue, &event, 0);
}

#else // (defined(ENABLE_HRM_TASK) && (ENABLE_HRM_TASK > 0U))

void hrm_pretask_init(void){}
void hrm_task(void *ignored){}
void hrm_event_turn_off(void){}
void hrm_event_turn_on(void){}

void hrm_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status){}

#endif // (defined(ENABLE_HRM_TASK) && (ENABLE_HRM_TASK > 0U))
