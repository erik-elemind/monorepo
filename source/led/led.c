

#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "loglevels.h"
#include "config.h"
#include "led.h"

#include "anim.h"
#include "audio.h"

#define LED_EVENT_QUEUE_SIZE 10

static const char *TAG = "led";	// Logging prefix for this module

//
// Task events:
//
typedef enum
{
  LED_EVENT_ENTER_STATE,	// (used for state transitions)
  LED_EVENT_TYPE1,
  LED_EVENT_TYPE2,
  LED_EVENT_TYPE3,
} led_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  led_event_type_t type;
  void *user_data;
} led_event_t;


//
// State machine states:
//
typedef enum
{
  LED_STATE_STANDBY,
  LED_STATE_STATE1,
  LED_STATE_STATE2,
  LED_STATE_STATE3,
} led_state_t;

//
// Global context data:
//
typedef struct
{
  led_state_t state;
  // TODO: Other "global" vars shared between events or states goes here.
} led_context_t;

static led_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[LED_EVENT_QUEUE_SIZE*sizeof(led_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(led_event_t *event);


// For logging and debug:
static const char *
led_state_name(led_state_t state)
{
  switch (state) {
    case LED_STATE_STANDBY: return "LED_STATE_STANDBY";
    case LED_STATE_STATE1: return "LED_STATE_STATE1";
    case LED_STATE_STATE2: return "LED_STATE_STATE2";
    case LED_STATE_STATE3: return "LED_STATE_STATE3";
    default:
      break;
  }
  return "LED_STATE UNKNOWN";
}

static const char *
led_event_type_name(led_event_type_t event_type)
{
  switch (event_type) {
    case LED_EVENT_ENTER_STATE: return "LED_EVENT_ENTER_STATE";
    case LED_EVENT_TYPE1: return "LED_EVENT_TYPE1";
    case LED_EVENT_TYPE2: return "LED_EVENT_TYPE2";
    case LED_EVENT_TYPE3: return "LED_EVENT_TYPE3";
    default:
      break;
  }
  return "LED_EVENT UNKNOWN";
}

void
led_event_type1(void)
{
  led_event_t event = {.type = LED_EVENT_TYPE1, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
led_event_type2(void)
{
  led_event_t event = {.type = LED_EVENT_TYPE2, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
led_event_type3(void)
{
  led_event_t event = {.type = LED_EVENT_TYPE3, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}


static void
log_event(led_event_t *event)
{
  switch (event->type) {
    default:
//      LOGV(TAG, "[%s] Event: %s\n\r", led_state_name(g_context.state), led_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(led_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s\n\r", led_state_name(g_context.state), led_event_type_name(event->type));
}

static void
set_state(led_state_t state)
{
//  LOGD(TAG, "[%s] -> [%s]\n\r", led_state_name(g_context.state), led_state_name(state));

  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  led_event_t event = { LED_EVENT_ENTER_STATE, (void *) state };
  handle_event(&event);
}



//
// Event handlers for the various application states:
//

static void
handle_state_standby(led_event_t *event)
{

  switch (event->type) {
    case LED_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      // Turn Off LEDs
      BOARD_SetRGB(0,0,0);
      break;

    case LED_EVENT_TYPE1:
      set_state(LED_STATE_STATE1);
      break;

    case LED_EVENT_TYPE2:
      set_state(LED_STATE_STATE1);
      break;

    case LED_EVENT_TYPE3:
      set_state(LED_STATE_STATE1);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}


static void
handle_state_state1(led_event_t *event)
{

  switch (event->type) {
    case LED_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      // Turn on red LED
      BOARD_SetRGB(100,0,0);
      break;

    case LED_EVENT_TYPE1:
      set_state(LED_STATE_STATE2);
      break;

    case LED_EVENT_TYPE2:
      set_state(LED_STATE_STATE2);
      break;

    case LED_EVENT_TYPE3:
      set_state(LED_STATE_STATE2);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}


static void
handle_state_state2(led_event_t *event)
{

  switch (event->type) {
    case LED_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      // Turn on green LED
      BOARD_SetRGB(0,100,0);
      break;

    case LED_EVENT_TYPE1:
      set_state(LED_STATE_STATE3);
      break;

    case LED_EVENT_TYPE2:
      set_state(LED_STATE_STATE3);
      break;

    case LED_EVENT_TYPE3:
      set_state(LED_STATE_STATE3);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}


static void
handle_state_state3(led_event_t *event)
{

  switch (event->type) {
    case LED_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      // Turn on blue LED
      BOARD_SetRGB(0,0,100);
      break;

    case LED_EVENT_TYPE1:
      set_state(LED_STATE_STANDBY);
      break;

    case LED_EVENT_TYPE2:
      set_state(LED_STATE_STANDBY);
      break;

    case LED_EVENT_TYPE3:
      set_state(LED_STATE_STANDBY);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}


static void
handle_event(led_event_t *event)
{
  switch (g_context.state) {
    case LED_STATE_STANDBY:
      handle_state_standby(event);
      break;

    case LED_STATE_STATE1:
      handle_state_state1(event);
      break;

    case LED_STATE_STATE2:
      handle_state_state2(event);
      break;

    case LED_STATE_STATE3:
      handle_state_state3(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown led state: %d\n\r", (int) g_context.state);
      break;
  }
}

void
led_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(LED_EVENT_QUEUE_SIZE,sizeof(led_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "led_event_queue");

  anim_init();
}


static void
task_init()
{
  // Any post-scheduler init goes here.

  set_state(LED_STATE_STANDBY);

  LOGV(TAG, "Task launched. Entering event loop.\n\r");
}

void
led_task(void *ignored)
{
  task_init();

  led_event_t event;
  int flag = 0;

  while (1) {

#if 0
	// ToDo: Replace/Delete the following debug code ----->
	vTaskDelay(200);

    // Make the LEDs cycle from Red->Green->Blue.
    event.type      = LED_EVENT_TYPE1;
    event.user_data = (void *) 0;
    // <------------ Replace/Delete
#else
    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);
#endif
    log_event(&event);

    handle_event(&event);
  }
}

// Animation pattern selector.

// Do a smooth transition between colors v1 and v2 over ms
// milliseconds, and then back again over ms milliseconds.  Repeats
// continually.

// v1 & v2 format is the same as Board_SetRGB32: 0x00RRGGBB
static void anim_pulse(uint32_t v1, uint32_t v2, uint32_t ms) {
  static animation_t anim[3][2];
  
  for(int i = 0; i < 3; i++) {
    anim[i][0].from = (v1 >> (8 * i)) & 0xff;
    anim[i][1].to = (v1 >> (8 * i)) & 0xff;
    
    anim[i][1].to = (v2 >> (8 * i)) & 0xff;
    anim[i][0].from = (v2 >> (8 * i)) & 0xff;
    
    anim[i][0].duration_in_ms = ms;
    anim[i][1].duration_in_ms = ms;

    // Shift amounts for Board_SetRGB32.
    anim[i][0].user_data = (void*)(i*8);
    anim[i][1].user_data = (void*)(i*8);

    anim_channel_start(i, anim[i], 2, true);
  }
}

// Callback to set animation color.

void anim_set_value(int value, void *user_data) {
  static uint32_t color_triple = 0;
  color_triple &= ~(0xff << (int)user_data);
  color_triple |= value << (int)user_data;
  BOARD_SetRGB32(color_triple);
}

 char* get_pattern_name(led_pattern_t pattern) {
  switch (pattern) {
   case LED_OFF: return "LED_OFF";
   case LED_ON:  return "LED_ON";
   case LED_THERAPY:  return "LED_THERAPY";
   case LED_CHARGING:  return "LED_CHARGING";
   case LED_CHARGED:  return "LED_CHARGED";
   case LED_CHARGE_FAULT:  return "LED_CHARGE_FAULT";
   case LED_RED:  return "LED_RED";
   case LED_POWER_GOOD: return "LED_POWER_GOOD";
   case LED_POWER_LOW: return "LED_POWER_LOW";
   default: return "UNKNOWN LED PATTERN";
  }
}

// FF3: Note colors are RGY, pre-FF3 are RGB.
static led_pattern_t cur_state;
void
set_led_state(led_pattern_t led_pattern)
{
  LOGD(TAG, "Setting led state to %s\n\r", get_pattern_name(led_pattern));
  
  // Don't reset the animation progress if the state is the same
  if (cur_state == led_pattern) {
    return;
  }
  cur_state = led_pattern;
  
  anim_channel_stop_all();
  switch (led_pattern) {
    case LED_OFF:
      BOARD_SetRGB32(0x000000);
      break;
    case LED_ON:
      BOARD_SetRGB32(0x0000aa);
      break;
    case LED_RED:
      BOARD_SetRGB32(0xFF0000);
      break;
    case LED_THERAPY:
      anim_pulse(0x000000, 0x00aa00, 1000);
      break;
    case LED_CHARGING:
      anim_pulse(0x000000, 0x885500, 1000);
      break;
    case LED_CHARGED:
      BOARD_SetRGB32(0x00ff00);
      break;
    case LED_CHARGE_FAULT:
      anim_pulse(0x000000, 0xaaaa00, 200);
      break;
    case LED_POWER_GOOD:
      BOARD_SetRGB32(0x00aa00);
      break;
    case LED_POWER_LOW:
      BOARD_SetRGB32(0xaa0000);
      break;
    default:
      LOGE(TAG, "set_led_state(): Unknown LED state!\n\r");
      break;
  }
}

