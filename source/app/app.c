/*
 * app.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: August, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Morpheus Application task implementation.
 *
 * Defines the app state machine, which manages overall device
 * behavior.
 *
 * We start out in APP_STATE_BOOT_UP at power-on; other tasks
 * are responsible for generating the appropriate events given the
 * state we find ourselves in when the device is turned on. For
 * example, the system monitor task will generate a
 * APP_EVENT_CHARGER_PLUGGED event at startup if the charger is
 * plugged in
 *
 * We don't start in _POWER_OFF, since if the power is off the LPC
 * isn't running--_POWER_OFF is an enter-only state. While the
 * processor does technically wake up in the _POWER_OFF state, we
 * immediately trigger a software reset to avoid any issues with
 * restoring all of the peripheral state that was lost at power-off.
 */
#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "ble.h"
#include "utils.h"
#include "loglevels.h"
#include "config.h"
#include "system_monitor.h"
#include "app.h"
#include "led.h"
#include "audio_task.h"
#include "battery.h"
#include "fsl_power.h"
#include "pmic_pca9420.h"
#include "../interpreter/interpreter.h"
#include "memfault/metrics/metrics.h"
#include "pmic_pca9420.h"
#include "memfault_commands.h"
#include "ymodem.h"

#if (defined(ENABLE_APP_TASK) && (ENABLE_APP_TASK > 0U))

#define APP_EVENT_QUEUE_SIZE 10
#define POWER_GOOD_BATT_THRESHOLD 15

static const char *TAG = "app";	// Logging prefix for this module

static uint32_t systemTickControl;

//
// Application events (high-level events for business logic):
//
typedef enum
{
  APP_EVENT_ENTER_STATE,    // (used for state transitions)

  APP_EVENT_POWER_BUTTON_DOWN,
  APP_EVENT_POWER_BUTTON_UP,
  APP_EVENT_POWER_BUTTON_CLICK,
  APP_EVENT_POWER_BUTTON_DOUBLE_CLICK,
  APP_EVENT_POWER_BUTTON_LONG_CLICK,

  APP_EVENT_VOLUP_BUTTON_DOWN,
  APP_EVENT_VOLUP_BUTTON_UP,
  APP_EVENT_VOLUP_BUTTON_CLICK,
  APP_EVENT_VOLDN_BUTTON_DOWN,
  APP_EVENT_VOLDN_BUTTON_UP,
  APP_EVENT_VOLDN_BUTTON_CLICK,

  APP_EVENT_BLE_ACTIVITY,
  APP_EVENT_RTC_ACTIVITY,
  APP_EVENT_BUTTON_ACTIVITY,
  APP_EVENT_SHELL_ACTIVITY,
  APP_EVENT_SLEEP_TIMEOUT,
  APP_EVENT_BLE_OFF_TIMEOUT,

  APP_EVENT_CHARGER_PLUGGED,
  APP_EVENT_CHARGER_UNPLUGGED,
  APP_EVENT_CHARGE_COMPLETE,
  APP_EVENT_CHARGE_FAULT,

  APP_EVENT_DEVMODE_ENABLE,
  APP_EVENT_DEVMODE_DISABLE,
} app_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  app_event_type_t type;
  void *user_data;
} app_event_t;


//
// Application state machine states:
//
typedef enum
{
  APP_STATE_INVALID = 0,
  APP_STATE_BOOT_UP,
  APP_STATE_ON,
  APP_STATE_SLEEP,
  APP_STATE_CHARGER_ATTACHED,
} app_state_t;


//
// Global application data (any business logic variables go here).
//
typedef struct
{
  app_state_t state;
  TimerHandle_t sleep_timer_handle;
  StaticTimer_t sleep_timer_struct;
  TimerHandle_t ble_off_timer_handle;
  StaticTimer_t ble_off_timer_struct;
} app_context_t;


//
// Constants
//
//#define TEST_TIMEOUTS // Uncomment to select shorter timeouts for testing
#ifndef TEST_TIMEOUTS
enum {
  SLEEP_TIMEOUT_MS = SECONDS_TO_MS(10),
  BLE_OFF_TIMEOUT_MS = MINUTES_TO_MS(5),
  LED_OFF_TIMEOUT_MS = SECONDS_TO_MS(5), // LED timer specifically for Sleep Session LED behavior
};
#else
enum {
  SLEEP_TIMEOUT_MS = SECONDS_TO_MS(30),
  BLE_OFF_TIMEOUT_MS = SECONDS_TO_MS(30),
};
#endif


//
// Global variables
//

static app_context_t g_app_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[APP_EVENT_QUEUE_SIZE*sizeof(app_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(app_event_t *event);


// For logging and debug:
static const char *
app_state_name(app_state_t state)
{
  switch (state) {
    case APP_STATE_BOOT_UP: return "APP_STATE_BOOT_UP";
    case APP_STATE_ON: return "APP_STATE_ON";
    case APP_STATE_CHARGER_ATTACHED: return "APP_STATE_CHARGER_ATTACHED";
    case APP_STATE_SLEEP: return "APP_STATE_SLEEP";
    default:
      break;
  }
  return "APP_STATE UNKNOWN";
}

static const char *
app_event_type_name(app_event_type_t event_type)
{
  switch (event_type) {
    case APP_EVENT_ENTER_STATE: return "APP_EVENT_ENTER_STATE";
    case APP_EVENT_POWER_BUTTON_DOWN: return "APP_EVENT_POWER_BUTTON_DOWN";
    case APP_EVENT_POWER_BUTTON_UP: return "APP_EVENT_POWER_BUTTON_UP";
    case APP_EVENT_POWER_BUTTON_CLICK: return "APP_EVENT_POWER_BUTTON_CLICK";
    case APP_EVENT_POWER_BUTTON_DOUBLE_CLICK: return "APP_EVENT_POWER_BUTTON_DOUBLE_CLICK";
    case APP_EVENT_POWER_BUTTON_LONG_CLICK: return "APP_EVENT_POWER_BUTTON_LONG_CLICK";
    case APP_EVENT_VOLUP_BUTTON_DOWN: return "APP_EVENT_VOLUP_BUTTON_DOWN";
    case APP_EVENT_VOLUP_BUTTON_UP: return "APP_EVENT_VOLUP_BUTTON_UP";
    case APP_EVENT_VOLUP_BUTTON_CLICK: return "APP_EVENT_VOLUP_BUTTON_CLICK";
    case APP_EVENT_VOLDN_BUTTON_DOWN: return "APP_EVENT_VOLDN_BUTTON_DOWN";
    case APP_EVENT_VOLDN_BUTTON_UP: return "APP_EVENT_VOLDN_BUTTON_UP";
    case APP_EVENT_VOLDN_BUTTON_CLICK: return "APP_EVENT_VOLDN_BUTTON_CLICK";
    case APP_EVENT_BLE_ACTIVITY: return "APP_EVENT_BLE_ACTIVITY";
    case APP_EVENT_RTC_ACTIVITY: return "APP_EVENT_RTC_ACTIVITY";
    case APP_EVENT_BUTTON_ACTIVITY: return "APP_EVENT_BUTTON_ACTIVITY";
    case APP_EVENT_SHELL_ACTIVITY: return "APP_EVENT_SHELL_ACTIVITY";
    case APP_EVENT_SLEEP_TIMEOUT: return "APP_EVENT_SLEEP_TIMEOUT";
    case APP_EVENT_BLE_OFF_TIMEOUT: return "APP_EVENT_BLE_OFF_TIMEOUT";
    case APP_EVENT_CHARGER_PLUGGED: return "APP_EVENT_CHARGER_PLUGGED";
    case APP_EVENT_CHARGER_UNPLUGGED: return "APP_EVENT_CHARGER_UNPLUGGED";
    case APP_EVENT_CHARGE_COMPLETE: return "APP_EVENT_CHARGE_COMPLETE";
    case APP_EVENT_CHARGE_FAULT: return "APP_EVENT_CHARGE_FAULT";

    default:
      break;
  }
  return "APP_EVENT UNKNOWN";
}

//
// Global function definitions
//

void
app_event_power_button_down(void)
{
  app_event_t event = {.type = APP_EVENT_POWER_BUTTON_DOWN, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_power_button_up(void)
{
  app_event_t event = {.type = APP_EVENT_POWER_BUTTON_UP, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_power_button_click(void)
{
  app_event_t event = {.type = APP_EVENT_POWER_BUTTON_CLICK, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_power_button_double_click(void)
{
  app_event_t event = {.type = APP_EVENT_POWER_BUTTON_DOUBLE_CLICK, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_power_button_long_click(void)
{
  app_event_t event = {.type = APP_EVENT_POWER_BUTTON_LONG_CLICK, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_volup_button_down(void)
{
#if 1
  app_event_t event = {.type = APP_EVENT_VOLUP_BUTTON_DOWN, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
#endif
}

void
app_event_volup_button_up(void)
{
  app_event_t event = {.type = APP_EVENT_VOLUP_BUTTON_UP, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_volup_button_click(void)
{
  app_event_t event = {.type = APP_EVENT_VOLUP_BUTTON_CLICK, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_voldn_button_down(void)
{
  app_event_t event = {.type = APP_EVENT_VOLDN_BUTTON_DOWN, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_voldn_button_up(void)
{
  app_event_t event = {.type = APP_EVENT_VOLDN_BUTTON_UP, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_voldn_button_click(void)
{
  app_event_t event = {.type = APP_EVENT_VOLDN_BUTTON_CLICK, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_rtc_activity(void)
{
  app_event_t event = {.type = APP_EVENT_RTC_ACTIVITY, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_ble_activity(void)
{
  app_event_t event = {.type = APP_EVENT_BLE_ACTIVITY, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_button_activity(void)
{
  app_event_t event = {.type = APP_EVENT_BUTTON_ACTIVITY, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_shell_activity(void)
{
  app_event_t event = {.type = APP_EVENT_SHELL_ACTIVITY, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
app_event_sleep_timeout(void)
{
#if (defined(ENABLE_APP_SLEEP_TIMER) && (ENABLE_APP_SLEEP_TIMER > 0U))
  app_event_t event = {.type = APP_EVENT_SLEEP_TIMEOUT, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
#endif
}

void
app_event_ble_off_timeout(void)
{
#if (defined(ENABLE_BLE_POWER_OFF_TIMER) && (ENABLE_BLE_POWER_OFF_TIMER > 0U))
  app_event_t event = {.type = APP_EVENT_BLE_OFF_TIMEOUT, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
#endif
}

void
app_event_charger_plugged(void)
{
#if (defined(ENABLE_CHARGER_PLUGGED_EVENTS) && (ENABLE_CHARGER_PLUGGED_EVENTS > 0U))
  app_event_t event = {.type = APP_EVENT_CHARGER_PLUGGED, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
#else
  set_led_state(LED_CHARGING);
#endif
}

void
app_event_charger_unplugged(void)
{
#if (defined(ENABLE_CHARGER_PLUGGED_EVENTS) && (ENABLE_CHARGER_PLUGGED_EVENTS > 0U))
  app_event_t event = {.type = APP_EVENT_CHARGER_UNPLUGGED, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
#else
  set_led_state(LED_OFF);
#endif
}

void
app_event_charge_complete(void)
{
#if (defined(ENABLE_CHARGER_PLUGGED_EVENTS) && (ENABLE_CHARGER_PLUGGED_EVENTS > 0U))
  app_event_t event = {.type = APP_EVENT_CHARGE_COMPLETE, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
#endif
}

void
app_event_charge_fault(void)
{
#if (defined(ENABLE_CHARGER_PLUGGED_EVENTS) && (ENABLE_CHARGER_PLUGGED_EVENTS > 0U))
  app_event_t event = {.type = APP_EVENT_CHARGE_FAULT, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
#endif
}


static void
log_event(app_event_t *event)
{
  switch (event->type) {
    // Suppressing charger events
    case APP_EVENT_CHARGER_PLUGGED: return;
    case APP_EVENT_CHARGER_UNPLUGGED: return;
    case APP_EVENT_CHARGE_COMPLETE: return;
    case APP_EVENT_CHARGE_FAULT: return;
    default:
      LOGV(TAG, "[%s] Event: %s", app_state_name(g_app_context.state), app_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(app_event_t *event)
{
  switch (event->type) {
    // Suppressing charger events
    case APP_EVENT_CHARGER_PLUGGED: return;
    case APP_EVENT_CHARGER_UNPLUGGED: return;
    case APP_EVENT_CHARGE_COMPLETE: return;
    case APP_EVENT_CHARGE_FAULT: return;
    default:
       LOGD(TAG, "[%s] Ignored Event: %s", app_state_name(g_app_context.state), app_event_type_name(event->type));
       break;
  }
}


static void
set_state(app_state_t state)
{
  LOGV(TAG, "[%s] -> [%s]", app_state_name(g_app_context.state), app_state_name(state));

  g_app_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  app_event_t event = { APP_EVENT_ENTER_STATE, (void *) state };
  handle_event(&event);
}

static void
restart_sleep_timer(void)
{
  // xTimerChangePeriod will start timer if it's not running already
  if (xTimerChangePeriod(g_app_context.sleep_timer_handle,
      pdMS_TO_TICKS(SLEEP_TIMEOUT_MS), 0) == pdFAIL) {
    LOGE(TAG, "Unable to start power off timer!");
  }
}

static void
stop_sleep_timer(void)
{
  if (xTimerStop(g_app_context.sleep_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to stop power off timer!");
  }
}

static void
restart_ble_off_timer(void)
{
  // xTimerChangePeriod will start timer if it's not running already
  if (xTimerChangePeriod(g_app_context.ble_off_timer_handle,
      pdMS_TO_TICKS(BLE_OFF_TIMEOUT_MS), 0) == pdFAIL) {
    LOGE(TAG, "Unable to start BLE off timer!");
  }
}

static void
stop_ble_off_timer(void)
{
  if (xTimerStop(g_app_context.ble_off_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to stop BLE off timer!");
  }
}

static void
set_led_by_charger_status(void)
{
  battery_charger_status_t status = pmic_battery_charger_get_status();

  switch (status)
    {
    case BATTERY_CHARGER_STATUS_ON_BATTERY:
      set_led_state(LED_RED);
      break;
    case BATTERY_CHARGER_STATUS_CHARGING:
      set_led_state(LED_CHARGING);
      break;
    case BATTERY_CHARGER_STATUS_CHARGE_COMPLETE:
      set_led_state(LED_CHARGED);
      break;
    case BATTERY_CHARGER_STATUS_FAULT:
      set_led_state(LED_CHARGE_FAULT);
      break;
    default:
      break;
    }
}

static void USB_ControllerSuspended(void)
{
    while (SYSCTL0->USBCLKSTAT & (SYSCTL0_USBCLKSTAT_HOST_NEED_CLKST_MASK))
    {
        __ASM("nop");
    }
    SYSCTL0->USBCLKCTRL |= SYSCTL0_USBCLKCTRL_POL_HOST_CLK_MASK;
}

static void 
presleep_tasks(void)
{
  LOGV(TAG, "Sleeping...");

  // flush memfault event logs to file system before Sleeping
  memfault_save_eventlog_chunks();

  // USB PreLowpowerMode
  USB_ControllerSuspended();

  if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
  {
      systemTickControl = SysTick->CTRL;
      SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
  }

  __disable_irq();
  NVIC_ClearPendingIRQ(USB_WAKEUP_IRQn);
  EnableIRQ(USB_WAKEUP_IRQn);
  SYSCTL0->STARTEN1 |= (SYSCTL0_STARTEN1_USB_IRQ(1) | SYSCTL0_STARTEN1_USB_NEEDCLK(1));

  BOARD_SetPmicVoltageBeforeDeepSleep();
  vTaskSuspendAll();
}

static void 
postsleep_tasks(void)
{
  BOARD_RestorePmicVoltageAfterDeepSleep();

  // USB PostLowpowerMode
  __enable_irq();
  SysTick->CTRL = systemTickControl;
  SYSCTL0->STARTEN1 &= ~(SYSCTL0_STARTEN1_USB_IRQ(1) | SYSCTL0_STARTEN1_USB_NEEDCLK(1));
  DisableIRQ(USB_WAKEUP_IRQn);

  xTaskResumeAll();

  // clean tasks when waking up
  interpreter_event_stop_script(false);

  LOGV(TAG, "Waking up!");
  set_state(APP_STATE_BOOT_UP);
}

//
// Event handlers for the various application states:
//
static void
handle_state_sleep(app_event_t *event)
{

  switch (event->type) {
    case APP_EVENT_ENTER_STATE:

    	// if therapy is not running AND if sync is not running AND if OTA is not signaled..
    	// go to sleep
      if ((interpreter_get_state() == INTERPRETER_STATE_STANDBY) &&
    		  (!ymodem_is_running()) &&
			  (!ble_is_ota_running()))
      {
        stop_sleep_timer();
        stop_ble_off_timer();
        set_led_state(LED_OFF);
        presleep_tasks();
        BOARD_EnterDeepSleep(APP_EXCLUDE_FROM_DEEPSLEEP);
        postsleep_tasks();
      }
      else // Therapy is on-going, set back to ON state
      {
    	  set_state(APP_STATE_ON);
      }
      break;

      /* Exit on activity. This would only occur if uC didn't actually Sleep */
    case APP_EVENT_BUTTON_ACTIVITY:
    case APP_EVENT_BLE_ACTIVITY:
    case APP_EVENT_SHELL_ACTIVITY:
    case APP_EVENT_RTC_ACTIVITY:
    	set_state(APP_STATE_ON);
    	break;

    default:
      log_event_ignored(event);
      break;
  }
}


static void
handle_state_boot_up(app_event_t *event)
{

  switch (event->type) {
    case APP_EVENT_ENTER_STATE:
      // TODO: add proper bootup/wakeup LED anim
      set_led_state(LED_OFF);
      vTaskDelay(100);
      set_led_state(LED_BLUE);
      vTaskDelay(200);

      // Advance to the next state.
      set_state(APP_STATE_ON);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_on(app_event_t *event)
{

  switch (event->type) {
      case APP_EVENT_ENTER_STATE:
        restart_sleep_timer();
        stop_ble_off_timer();
        break;

      case APP_EVENT_SLEEP_TIMEOUT:
        set_state(APP_STATE_SLEEP);
        break;

      case APP_EVENT_CHARGER_PLUGGED:
      case APP_EVENT_CHARGE_COMPLETE:
      case APP_EVENT_CHARGE_FAULT:
    	  set_state(APP_STATE_CHARGER_ATTACHED);
    	  break;

      case APP_EVENT_POWER_BUTTON_LONG_CLICK:
    	  set_state(APP_STATE_SLEEP);
    	  break;

      case APP_EVENT_BUTTON_ACTIVITY:
    	  restart_sleep_timer();
    	  break;

      case APP_EVENT_RTC_ACTIVITY:
            restart_sleep_timer();
            break;

      default:
        log_event_ignored(event);
        break;
    }
}


static void
handle_state_charger_attached(app_event_t *event)
{
	switch (event->type) {
		case APP_EVENT_ENTER_STATE:
			stop_sleep_timer();
			stop_ble_off_timer();
			break;

		case APP_EVENT_CHARGER_UNPLUGGED:
			set_state(APP_STATE_ON);
			break;

		default:
			log_event_ignored(event);
			break;
	}
}

static void
handle_event(app_event_t *event)
{
  // handle stateless events
    switch(event->type){
		case APP_EVENT_VOLUP_BUTTON_CLICK:
		  audio_volume_up();
		  memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(vol_up_button_presses), 1);
		  break;
		case APP_EVENT_VOLDN_BUTTON_CLICK:
		  audio_volume_down();
		  memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(vol_down_button_presses), 1);
		  break;
		case APP_EVENT_BLE_ACTIVITY:
		  restart_ble_off_timer();
		  restart_sleep_timer();
		  break;
		case APP_EVENT_BUTTON_ACTIVITY:
		  restart_sleep_timer();
		  break;
		case APP_EVENT_SHELL_ACTIVITY:
		case APP_EVENT_RTC_ACTIVITY:
		  restart_sleep_timer();
		  break;
		case APP_EVENT_BLE_OFF_TIMEOUT:
		  stop_ble_off_timer();
		  ble_power_off();
		  break;

		case APP_EVENT_CHARGER_PLUGGED:
		case APP_EVENT_CHARGER_UNPLUGGED:
		case APP_EVENT_CHARGE_COMPLETE:
		case APP_EVENT_CHARGE_FAULT:
		  // update LED state
		  set_led_by_charger_status();
		  break;

		default:
		  // try handlers below
		  break;
    }
    set_led_by_charger_status();

  switch (g_app_context.state) {
      case APP_STATE_SLEEP:
        handle_state_sleep(event);
        break;

      case APP_STATE_BOOT_UP:
        handle_state_boot_up(event);
        break;

      case APP_STATE_ON:
        handle_state_on(event);
        break;

      case APP_STATE_CHARGER_ATTACHED:
        handle_state_charger_attached(event);
        break;

      default:
        // (We should never get here.)
        LOGE(TAG, "Unknown app_state: %d", (int) g_app_context.state);
        break;
    }
}

static void
sleep_timeout(TimerHandle_t timer_handle)
{
  app_event_sleep_timeout();
}

static void
ble_off_timeout(TimerHandle_t timer_handle)
{
  app_event_ble_off_timeout();
}

void USB_WAKEUP_IRQHandler(void)
{
    NVIC_ClearPendingIRQ(USB_WAKEUP_IRQn);
}

void
app_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(APP_EVENT_QUEUE_SIZE,sizeof(app_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "app_event_queue");
}


static void
task_init()
{
  // Any post-scheduler init goes here.
  g_app_context.sleep_timer_handle = xTimerCreateStatic("APP_SLEEP",
     pdMS_TO_TICKS(SLEEP_TIMEOUT_MS), pdFALSE, NULL,
     sleep_timeout, &(g_app_context.sleep_timer_struct));

  g_app_context.ble_off_timer_handle = xTimerCreateStatic("APP_BLE_OFF",
    pdMS_TO_TICKS(BLE_OFF_TIMEOUT_MS), pdFALSE, NULL,
    ble_off_timeout, &(g_app_context.ble_off_timer_struct));

  set_state(APP_STATE_BOOT_UP);

  LOGV(TAG, "Task launched. Entering event loop.");
}

void
app_task(void *ignored)
{

  task_init();

  app_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

  }
}

#else // (defined(ENABLE_APP_TASK) && (ENABLE_APP_TASK > 0U))

void app_pretask_init(void){}
void app_task(void *ignored){}

void app_event_power_button_down(void){}
void app_event_power_button_up(void){}
void app_event_power_button_click(void){}
void app_event_power_button_double_click(void){}
void app_event_power_button_long_click(void){}
void app_event_volup_button_down(void){}
void app_event_volup_button_up(void){}
void app_event_volup_button_click(void){}
void app_event_voldn_button_down(void){}
void app_event_voldn_button_up(void){}
void app_event_voldn_button_click(void){}
void app_event_ble_therapy_start(therapy_type_t therapy){}
void app_event_ble_activity(void){}
void app_event_button_activity(void){}
void app_event_shell_activity(void){}
void app_event_sleep_timeout(void){}
void app_event_ble_off_timeout(void){}
void app_event_charger_plugged(void){}
void app_event_charger_unplugged(void){}
void app_event_charge_complete(void){}
void app_event_charge_fault(void){}


#endif // (defined(ENABLE_APP_TASK) && (ENABLE_APP_TASK > 0U))
