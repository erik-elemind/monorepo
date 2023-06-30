
#include <stdlib.h>
#include <stdbool.h>
#include <math.h> // used for ceil, log10, pow

#include "board_config.h"

#include "fsl_power.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "config.h"
#include "app.h"
#include "led.h"
#include "system_watchdog.h"
#include "adc.h"
#include "battery.h"

#include "system_monitor.h"

#include "als.h"
#include "data_log.h"
#include "pin_mux.h"

// Reduce log level for this module
#define LOG_LEVEL_MODULE LOG_DEBUG
#include "loglevels.h"
#include "utils.h"


#if (defined(ENABLE_SYSTEM_MONITOR_TASK) && (ENABLE_SYSTEM_MONITOR_TASK > 0U))

#define SYSTEM_MONITOR_EVENT_QUEUE_SIZE 50

// Power off constants
#define APP_EXCLUDE_FROM_POWERDOWN (kPDRUNCFG_PD_LDOMEM | kPDRUNCFG_PD_FRO32K)
//#define APP_SRAM_RETAIN (LOWPOWER_SRAMRETCTRL_RETEN_RAMX2 | LOWPOWER_SRAMRETCTRL_RETEN_RAM00 | LOWPOWER_SRAMRETCTRL_RETEN_RAM01 | LOWPOWER_SRAMRETCTRL_RETEN_RAM10 | LOWPOWER_SRAMRETCTRL_RETEN_RAM20) // Doesn't work--not sure why
#define APP_SRAM_RETAIN (0x7FFF)

#define ENABLE_TIMER_BATTERY_MONITOR (1U)

static const char *TAG = "system_monitor";	// Logging prefix for this module

//
// State machine states:
//
typedef enum
{
  SYSTEM_MONITOR_STATE_STANDBY,
} system_monitor_state_t;

//
// Task events:
//

typedef enum
{
  SYSTEM_MONITOR_EVENT_ENTER_STATE,	// (used for state transitions)
  SYSTEM_MONITOR_EVENT_BATTERY,
  SYSTEM_MONITOR_EVENT_POWER_OFF,
  SYSTEM_MONITOR_EVENT_WWDT_FEED_TIMEOUT,
  SYSTEM_MONITOR_EVENT_ALS_START_SAMPLE,
  SYSTEM_MONITOR_EVENT_ALS_STOP,
  SYSTEM_MONITOR_EVENT_ALS_TIMEOUT,
  SYSTEM_MONITOR_EVENT_MIC_START_SAMPLE,
  SYSTEM_MONITOR_EVENT_MIC_START_THRESH,
  SYSTEM_MONITOR_EVENT_MIC_STOP,
  SYSTEM_MONITOR_EVENT_MIC_WAKE,
  SYSTEM_MONITOR_EVENT_MIC_TIMEOUT,
} system_monitor_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  system_monitor_event_type_t type;
  union{
    system_monitor_state_t state;
    struct{
      unsigned int sample_period_ms;
      unsigned int num_samples;
    } sensor;
  } data;
} system_monitor_event_t;

typedef enum{
  ALS_STATE_STOPPED,
  ALS_STATE_SAMPLING,
} als_state_t;

typedef enum{
  MIC_STATE_STOPPED,
  MIC_STATE_SAMPLING,
  MIC_STATE_WAIT_WAKE,
} mic_state_t;

//
// Global context data:
//
typedef struct
{
  system_monitor_state_t state;
  TimerHandle_t wwdt_feed_timer_handle;
  StaticTimer_t wwdt_feed_timer_struct;
  
  int32_t battery_adc; // percent

  als_state_t als_state;
  TimerHandle_t als_timer_handle;
  StaticTimer_t als_timer_struct;
  unsigned int als_timer_ms;
  unsigned int als_num_samples;

  mic_state_t mic_state;
  TimerHandle_t mic_timer_handle;
  StaticTimer_t mic_timer_struct;
  unsigned int mic_timer_ms;
  unsigned int mic_wake_max_num_samples;
  unsigned int mic_num_samples;

  // TODO: Other "global" vars shared between events or states goes here.
} system_monitor_context_t;

static system_monitor_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[SYSTEM_MONITOR_EVENT_QUEUE_SIZE*sizeof(system_monitor_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(system_monitor_event_t *event);

// For logging and debug:
static const char *
system_monitor_state_name(system_monitor_state_t state)
{
  switch (state) {
    case SYSTEM_MONITOR_STATE_STANDBY:
      return "SYSTEM_MONITOR_STATE_STANDBY";
    default:
      break;
  }
  return "SYSTEM_MONITOR_STATE UNKNOWN";
}

static const char *
system_monitor_event_type_name(system_monitor_event_type_t event_type)
{
  switch (event_type) {
    case SYSTEM_MONITOR_EVENT_ENTER_STATE:
      return "SYSTEM_MONITOR_EVENT_ENTER_STATE";
    case SYSTEM_MONITOR_EVENT_BATTERY:
      return "SYSTEM_MONITOR_EVENT_BATTERY";
    case SYSTEM_MONITOR_EVENT_POWER_OFF:
      return "SYSTEM_MONITOR_EVENT_POWER_OFF";
    case SYSTEM_MONITOR_EVENT_WWDT_FEED_TIMEOUT:
      return "SYSTEM_MONITOR_EVENT_WWDT_FEED_TIMEOUT";
    case SYSTEM_MONITOR_EVENT_ALS_START_SAMPLE:
      return "SYSTEM_MONITOR_EVENT_ALS_START_SAMPLE";
    case SYSTEM_MONITOR_EVENT_ALS_STOP:
      return "SYSTEM_MONITOR_EVENT_ALS_STOP";
    case SYSTEM_MONITOR_EVENT_ALS_TIMEOUT:
      return "SYSTEM_MONITOR_EVENT_ALS_TIMEOUT";
    case SYSTEM_MONITOR_EVENT_MIC_START_SAMPLE:
      return "SYSTEM_MONITOR_EVENT_MIC_START_SAMPLE";
    case SYSTEM_MONITOR_EVENT_MIC_START_THRESH:
      return "SYSTEM_MONITOR_EVENT_MIC_START_THRESH";
    case SYSTEM_MONITOR_EVENT_MIC_STOP:
      return "SYSTEM_MONITOR_EVENT_MIC_STOP";
    case SYSTEM_MONITOR_EVENT_MIC_WAKE:
      return "SYSTEM_MONITOR_EVENT_MIC_WAKE";
    case SYSTEM_MONITOR_EVENT_MIC_TIMEOUT:
      return "SYSTEM_MONITOR_EVENT_MIC_TIMEOUT";
    default:
      break;
  }
  return "SYSTEM_MONITOR_EVENT UNKNOWN";
}

void
system_monitor_event_battery(void)
{
  system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_BATTERY
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}


void
system_monitor_event_power_off()
{
  system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_POWER_OFF
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}


/********************************/
// ALS

void system_monitor_event_als_start_sample(unsigned int sample_period_ms){
  system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_ALS_START_SAMPLE,
    .data = { .sensor = {.sample_period_ms = sample_period_ms } }
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void system_monitor_event_als_stop(void){
  static const system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_ALS_STOP
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void sample_and_log_als(void) {
  g_context.als_num_samples++;

  int err;
  float lux;
  // Finished a sample.
  err = als_get_lux(&lux);
  if (!err) {
    data_log_als(g_context.als_num_samples, lux);
  }
}

static void
handle_als_start_sample(unsigned int sample_period_ms){
  // init als state
  g_context.als_state = ALS_STATE_SAMPLING;
  g_context.als_num_samples = 0;
  // power on the als
  als_start();
  // collect first sample
  sample_and_log_als();
  // update and start timer
  xTimerChangePeriod(g_context.als_timer_handle, pdMS_TO_TICKS(sample_period_ms), portMAX_DELAY );
  vTimerSetReloadMode(g_context.als_timer_handle, true);
  xTimerStart(g_context.als_timer_handle, portMAX_DELAY);
}

static void
handle_als_stop(){
  // init microphone state
  g_context.als_state = ALS_STATE_STOPPED;
  g_context.als_num_samples = 0;
  // stop the als
  als_stop();
  // stop the timer
  xTimerStop(g_context.als_timer_handle, portMAX_DELAY);
}

static void
handle_als_timeout(){
  if (g_context.als_state == ALS_STATE_SAMPLING) {
    // Finished a sample.
    sample_and_log_als();
  }
}


/********************************/
// MIC

void system_monitor_event_mic_start_sample(unsigned int sample_period_ms){
  system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_MIC_START_SAMPLE,
    .data = { .sensor = {.sample_period_ms = sample_period_ms } }
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void system_monitor_event_mic_start_thresh(unsigned int sample_period_ms_, unsigned int num_samples_){
   system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_MIC_START_THRESH,
    .data = { .sensor = {.sample_period_ms = sample_period_ms_, .num_samples = num_samples_ } }
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void system_monitor_event_mic_stop(void){
  static const system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_MIC_STOP
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

int32_t
system_monitor_event_mic_read_once(void) {
  // Wake up the mic
  GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(1)); // Startup delay is 100us per datasheet.

  int32_t mic = adc_read(ADC_MICROPHONE);

  // put the mic to sleep
  GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 1);
  
  return mic;
}

void
system_monitor_event_mic_from_isr(void) {
#if MIC_SAMPLES_BEFORE_SLEEP > 0
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static const system_monitor_event_t event = {
      .type = SYSTEM_MONITOR_EVENT_MIC_WAKE
    };
    xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#endif // MEMS_SAMPLES_BEFORE_SLEEP > 0
}

static void sample_and_log_mic(void) {
  g_context.mic_num_samples++;

  int32_t mic = adc_read(ADC_MICROPHONE);
//  LOGD(TAG, "MIC reading: %li", mic);
  data_log_mic(g_context.mic_num_samples, mic);
}

static void
handle_mic_start_sample(unsigned int sample_period_ms){
  // init microphone state
  g_context.mic_state = MIC_STATE_SAMPLING;
  g_context.mic_num_samples = 0;
  // power on the microphone
  GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(1)); // Startup delay is 100us per datasheet.
  // collect first sample
  sample_and_log_mic();
  // update and start timer
  xTimerChangePeriod(g_context.mic_timer_handle, pdMS_TO_TICKS(sample_period_ms), portMAX_DELAY );
  vTimerSetReloadMode(g_context.mic_timer_handle, true);
  xTimerStart(g_context.mic_timer_handle, portMAX_DELAY);
}

static void
handle_mic_start_thresh(unsigned int sample_period_ms, unsigned int num_samples){
  // init microphone state
  g_context.mic_state = MIC_STATE_WAIT_WAKE;
  g_context.mic_wake_max_num_samples = num_samples;
  g_context.mic_num_samples = 0;
  // power off the microphone
  GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 1);
  // update and stop the timer
  xTimerStop(g_context.mic_timer_handle, portMAX_DELAY);
  xTimerChangePeriod(g_context.mic_timer_handle, pdMS_TO_TICKS(sample_period_ms), portMAX_DELAY );
  vTimerSetReloadMode(g_context.mic_timer_handle, false);
}

static void
handle_mic_stop(){
  // init microphone state
  g_context.mic_state = MIC_STATE_STOPPED;
  g_context.mic_num_samples = 0;
  // power off the microphone
  GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 1);
  // stop the timer
  xTimerStop(g_context.mic_timer_handle, portMAX_DELAY);
}

void
handle_mic_timeout(void) {
  switch(g_context.mic_state){
  case MIC_STATE_SAMPLING:
    sample_and_log_mic();
    break;
  case MIC_STATE_WAIT_WAKE:
    if (g_context.mic_wake_max_num_samples != 0 &&
        g_context.mic_num_samples >= g_context.mic_wake_max_num_samples) {
      g_context.mic_num_samples = 0;
      // power off the microphone
      GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 1);
    } else {
      sample_and_log_mic();
      xTimerStart(g_context.mic_timer_handle, 0);
    }
    break;
  case MIC_STATE_STOPPED:
    // do nothing
    break;
  }
}

void
handle_mic_wakeup(void) {
  if ( g_context.mic_state == MIC_STATE_WAIT_WAKE ){
    // power on the microphone
    GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1)); // Startup delay is 100us per datasheet.
    // collect first sample
    sample_and_log_mic();
    // update and start timer
    xTimerStart(g_context.mic_timer_handle, 0);
  }
}

static void
log_event(system_monitor_event_t *event)
{
  switch (event->type) {
    case SYSTEM_MONITOR_EVENT_WWDT_FEED_TIMEOUT:
      // suppress outputs from watchdog feeding event
    break;
#if (defined(ENABLE_TIMER_BATTERY_MONITOR) && (ENABLE_TIMER_BATTERY_MONITOR > 0U))
    case SYSTEM_MONITOR_EVENT_BATTERY:
    break;
#endif
    default:
      LOGV(TAG, "[%s] Event: %s",
        system_monitor_state_name(g_context.state),
        system_monitor_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(system_monitor_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s",
    system_monitor_state_name(g_context.state),
    system_monitor_event_type_name(event->type));
}

static void
set_state(system_monitor_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]",
    system_monitor_state_name(g_context.state),
    system_monitor_state_name(state));

  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_ENTER_STATE,
    .data = {.state = state}
  };
  handle_event(&event);
}

static void
battery_level_get(void) {
  g_context.battery_adc = battery_get_percent();
  LOGV(TAG, "Battery level: %li", g_context.battery_adc);
}


/** Got a battery event--set LEDs appropriately. */
static void
handle_battery_event(void)
{
  static battery_charger_status_t prev_status = -1;

  battery_charger_status_t battery_status = pmic_battery_charger_get_status();

  //battery_level_get(); // ToDo: Need to bring up proper battery reading

  // Don't update the charger status if it is the same.
  if (prev_status == battery_status) {
    return;
  }
  prev_status = battery_status;

  switch (battery_status) {
    case BATTERY_CHARGER_STATUS_ON_BATTERY:
      app_event_charger_unplugged();
      break;

    case BATTERY_CHARGER_STATUS_CHARGING:
      app_event_charger_plugged();
      break;

    case BATTERY_CHARGER_STATUS_CHARGE_COMPLETE:
      app_event_charger_plugged();
      app_event_charge_complete();
      break;

    case BATTERY_CHARGER_STATUS_FAULT:
      app_event_charger_plugged();
      app_event_charge_fault();
      break;

    default:
      LOGE(TAG, "Error: Unknown battery status value");
      break;
  }
}

/** Got a power off event--shut down the LPC. */
static void
handle_power_off_event(void)
{
#if 0
  /* Disable battery watchdog (otherwise watchdog timeout interrupt
     will wake us up). */
  status_t status = battery_charger_disable_wdog(&g_battery_charger_handle);
  if (status != kStatus_Success) {
    LOGE(TAG, "battery_charger_disable_wdog(): Error: %ld", status);
  }

  PMC->RESETCTRL |= PMC_RESETCTRL_SWRRESETENABLE(1);

  // Turn off LPC
  POWER_EnterPowerDown(APP_EXCLUDE_FROM_POWERDOWN, APP_SRAM_RETAIN,
    WAKEUP_GPIO_GLOBALINT1, 1);

  // Reset on wakeup
  // This should run upon wake
  SYSCON->SWR_RESET = 0x5A000001;
#endif
}

//
// Event handlers for the various application states:
//
static void
handle_state_standby(system_monitor_event_t *event)
{
  switch (event->type) {
    case SYSTEM_MONITOR_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      break;

    case SYSTEM_MONITOR_EVENT_BATTERY:
      handle_battery_event();
      break;

    case SYSTEM_MONITOR_EVENT_POWER_OFF:
          handle_power_off_event();
          break;

	case SYSTEM_MONITOR_EVENT_WWDT_FEED_TIMEOUT:
	#if (defined(ENABLE_SYSTEM_WATCHDOG) && (ENABLE_SYSTEM_WATCHDOG > 0U))
	  // feed watchdog
	  system_watchdog_feed();
	  // restart timer
	//    restart_wwdt_feed_timer();
	#endif
	  return;

	case SYSTEM_MONITOR_EVENT_ALS_START_SAMPLE:
	  handle_als_start_sample(event->data.sensor.sample_period_ms);
	  break;

	case SYSTEM_MONITOR_EVENT_ALS_STOP:
	  handle_als_stop();
	  break;

	case SYSTEM_MONITOR_EVENT_ALS_TIMEOUT:
	  handle_als_timeout();
	  break;

	case SYSTEM_MONITOR_EVENT_MIC_START_SAMPLE:
	  handle_mic_start_sample(event->data.sensor.sample_period_ms);
	  break;

	case SYSTEM_MONITOR_EVENT_MIC_START_THRESH:
	  handle_mic_start_thresh(event->data.sensor.sample_period_ms, event->data.sensor.num_samples);
	  break;

	case SYSTEM_MONITOR_EVENT_MIC_STOP:
	  handle_mic_stop();
	  break;

	case SYSTEM_MONITOR_EVENT_MIC_WAKE:
	  handle_mic_wakeup();
	  break;

	case SYSTEM_MONITOR_EVENT_MIC_TIMEOUT:
	  handle_mic_timeout();
	  break;
    default:
      log_event_ignored(event);
      break;
  }
}

#if (defined(ENABLE_SYSTEM_WATCHDOG) && (ENABLE_SYSTEM_WATCHDOG > 0U))

//static void
//restart_wwdt_feed_timer(void)
//{
//  // xTimerChangePeriod will start timer if it's not running already
//  if (xTimerChangePeriod(g_context.wwdt_feed_timer_handle,
//      pdMS_TO_TICKS(system_watchdog_reset_ms()), 0) == pdFAIL) {
//    LOGE(TAG, "Unable to start WWDT feed timer!");
//  }
//}

#if (defined(MEMFAULT_TEST_COMMANDS) && (MEMFAULT_TEST_COMMANDS > 0U))
void stop_wwdt_feed_timer(void)
{
  if (xTimerStop(g_context.wwdt_feed_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to stop WWDT feed timer!");
  }
}
#endif

static void
system_monitor_event_wwdt_feed_timeout(void)
{
  system_monitor_event_t event = {.type = SYSTEM_MONITOR_EVENT_WWDT_FEED_TIMEOUT };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
wwdt_feed_timeout(TimerHandle_t timer_handle)
{
  system_monitor_event_wwdt_feed_timeout();
}
#endif

static void
als_timeout(TimerHandle_t timer_handle){
  system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_ALS_TIMEOUT
  };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
mic_timeout(TimerHandle_t timer_handle){
  static const system_monitor_event_t event = {
    .type = SYSTEM_MONITOR_EVENT_MIC_TIMEOUT
  };
  xQueueSend(g_event_queue, &event, 0);
}

static void
handle_event(system_monitor_event_t *event)
{
  // handle events with state:
  switch (g_context.state) {
    case SYSTEM_MONITOR_STATE_STANDBY:
      handle_state_standby(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown system_monitor state: %d", (int) g_context.state);
      break;
  }
}

#if (defined(ENABLE_TIMER_BATTERY_MONITOR) && (ENABLE_TIMER_BATTERY_MONITOR > 0U))
TimerHandle_t g_battery_event_timer_handle = NULL;
StaticTimer_t g_battery_event_timer_struct;
static void
battery_event_timer_timeout(TimerHandle_t timer_handle){
  system_monitor_event_battery();
}
#endif

void
system_monitor_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(SYSTEM_MONITOR_EVENT_QUEUE_SIZE,sizeof(system_monitor_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "system_monitor_event_queue");
}

static void
task_init()
{
  // Any post-scheduler init goes here.

  // Initialize Watchdog
#if (defined(ENABLE_SYSTEM_WATCHDOG) && (ENABLE_SYSTEM_WATCHDOG > 0U))
  system_watchdog_init();

  g_context.wwdt_feed_timer_handle = xTimerCreateStatic("WWDT_FEED",
    pdMS_TO_TICKS(system_watchdog_reset_ms()), pdTRUE, NULL,
    wwdt_feed_timeout, &(g_context.wwdt_feed_timer_struct));
  xTimerStart(g_context.wwdt_feed_timer_handle, portMAX_DELAY);

//  restart_wwdt_feed_timer();
#endif

  // Initialize ADC for battery level reads
  adc_init();

  // Force a battery event to get power-on battery state
  handle_battery_event();

#if (defined(ENABLE_TIMER_BATTERY_MONITOR) && (ENABLE_TIMER_BATTERY_MONITOR > 0U))
  // start with a dummy time
  g_battery_event_timer_handle = xTimerCreateStatic("BATTERY_EVENT_TIMER_HANDLE",
    pdMS_TO_TICKS(1000), pdTRUE, NULL,
    battery_event_timer_timeout, &(g_battery_event_timer_struct));
  // TODO: do something with return status
  xTimerStart(g_battery_event_timer_handle,portMAX_DELAY);
#endif

  // Initialize ALS
  als_init();

  g_context.als_timer_ms = ALS_TIMER_MS;
  g_context.als_timer_handle = xTimerCreateStatic("ALS_TIMER", pdMS_TO_TICKS(ALS_TIMER_MS), pdTRUE, NULL, als_timeout,
						  &(g_context.als_timer_struct));

  // Initialize Microphone
  GPIO_PinWrite(GPIO, BOARD_INITPINS_MEMS_MODE_PORT, BOARD_INITPINS_MEMS_MODE_PIN, 0);
  g_context.mic_timer_handle = xTimerCreateStatic("MIC_TIMER", pdMS_TO_TICKS(MIC_TIMER_MS), pdFALSE, NULL, mic_timeout,
						  &(g_context.mic_timer_struct));

  set_state(SYSTEM_MONITOR_STATE_STANDBY);

  LOGV(TAG, "Task launched. Entering event loop.");
}

void
system_monitor_task(void *ignored)
{

  task_init();

  system_monitor_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

  }
}

#else // (defined(ENABLE_SYSTEM_MONITOR_TASK) && (ENABLE_SYSTEM_MONITOR_TASK > 0U))

/// Battery charger handle
battery_charger_handle_t g_battery_charger_handle;

void system_monitor_pretask_init(void){}
void system_monitor_task(void *ignored){}
void system_monitor_event_battery(void){}
void system_monitor_event_battery_from_isr(void){}
void system_monitor_event_power_off(void){}

void system_monitor_event_als_start_sample(unsigned int sample_period_ms){}
void system_monitor_event_als_stop(void){}

void system_monitor_event_mic_start_sample(unsigned int sample_period_ms){}
void system_monitor_event_mic_start_thresh(unsigned int sample_period_ms, unsigned int sample_num){}
void system_monitor_event_mic_stop(void){}
int32_t system_monitor_event_mic_read_once(void){ return 0; }
void system_monitor_event_mic_from_isr(void){}

#endif // (defined(ENABLE_SYSTEM_MONITOR_TASK) && (ENABLE_SYSTEM_MONITOR_TASK > 0U))
