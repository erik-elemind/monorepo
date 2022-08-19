/*
 * wavbuf.cpp
 *
 *  Created on: Sep 29, 2020
 *      Author: David Wang
 */

#include "noise_test.h"

#if (defined(ENABLE_NOISE_TEST) && (ENABLE_NOISE_TEST > 0U))

#include "config.h"
#include "flash_spi.h"
#include "system_monitor.h"
#include "loglevels.h"
#include "utils.h"

#define NOISE_TEST_EVENT_QUEUE_SIZE 2

static const char *TAG = "flash_test";   // Logging prefix for this module

//
// Task events:
//
typedef enum
{
  NOISE_TEST_EVENT_ENTER_STATE,  // (used for state transitions)
  NOISE_TEST_EVENT_FLASH_START,
  NOISE_TEST_EVENT_FLASH_STOP,
  NOISE_TEST_EVENT_FLASH_TICK
} noise_test_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  noise_test_event_type_t type;
  void *user_data;
} noise_test_event_t;

//
// State machine states:
//
typedef enum
{
  NOISE_TEST_STATE_IDLE,
  NOISE_TEST_STATE_FLASH_TEST,
} noise_test_state_t;

//
// Global context data:
//
typedef struct
{
  noise_test_state_t state;
  // The period at which the flash test should be run
  TickType_t flash_test_period_ticks;
  // Timer to periodically run a flash read or write operation.
  TimerHandle_t flash_test_timer;
  StaticTimer_t flash_test_timer_struct;
  // flash test data cache
  uint8_t* flash_test_data;
  // flash test cases index
  char flash_test_cases[NOISE_TEST_FLASH_MAX_NUM_CASES];
  // flash test cases index
  uint8_t flash_test_cases_index;
} noise_test_context_t;

typedef struct
{
  char cases[NOISE_TEST_FLASH_MAX_NUM_CASES];
} noise_test_event_flash_start_data_t;


static noise_test_context_t g_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[NOISE_TEST_EVENT_QUEUE_SIZE*sizeof(noise_test_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(noise_test_event_t *event);
void flash_test_timer_cb( TimerHandle_t xTimer );


// For logging and debug:
static const char *
noise_test_state_name(noise_test_state_t state)
{
  switch (state) {
    case NOISE_TEST_STATE_IDLE:          return "NOISE_TEST_STATE_IDLE";
    case NOISE_TEST_STATE_FLASH_TEST:    return "NOISE_TEST_STATE_FLASH_TEST";
    default:
      break;
  }
  return "NOISE_TEST_STATE UNKNOWN";
}

static const char *
noise_test_event_type_name(noise_test_event_type_t event_type)
{
  switch (event_type) {
    case NOISE_TEST_EVENT_ENTER_STATE:   return "NOISE_TEST_EVENT_ENTER_STATE";
    case NOISE_TEST_EVENT_FLASH_START:   return "NOISE_TEST_EVENT_FLASH_START";
    case NOISE_TEST_EVENT_FLASH_STOP:    return "NOISE_TEST_EVENT_FLASH_STOP";
    case NOISE_TEST_EVENT_FLASH_TICK:    return "NOISE_TEST_EVENT_FLASH_TICK";

    default:
      break;
  }
  return "NOISE_TEST_EVENT UNKNOWN";
}

/*****************************************************************************/
// Thread-safe functions

noise_test_event_flash_start_data_t g_noise_test_event_flash_start_data;
void noise_test_flash_start(char* cases)
{
  LOGV(TAG,"|%s|", cases);
  strcpy(g_noise_test_event_flash_start_data.cases, cases);
  LOGV(TAG,"copied:|%s|", g_noise_test_event_flash_start_data.cases);
  noise_test_event_t event = {.type = NOISE_TEST_EVENT_FLASH_START, .user_data = &g_noise_test_event_flash_start_data };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void noise_test_flash_stop()
{
  noise_test_event_t event = {.type = NOISE_TEST_EVENT_FLASH_STOP, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

/*****************************************************************************/
//

static void
noise_test_flash_cache_read()
{
  LOGV(TAG,"Read from flash internal page cache.");

#if 1
  status_t status;

  // Read data from page cache
  status = flash_spi_read_page_cache(&g_flash_spi_handle, 0,
      g_context.flash_test_data, FLASH_SPI_PAGE_SIZE);
  if (status != kStatus_Success) {
    LOGE(TAG,"Flash read page cache error: %ld\n", status);
    return;
  }
#endif

}

static void
noise_test_flash_cache_write()
{
  LOGV(TAG,"Write to flash internal page cache.");

#if 1
  status_t status;

  // Setup data
  uint16_t* p_data16 = (uint16_t*)g_context.flash_test_data;
  for (size_t i = 0; i < FLASH_SPI_PAGE_SIZE/sizeof(uint16_t); i++) {
    p_data16[i] = i*2;
  }

  // Write data to page cache
  status = flash_spi_write_page_cache(&g_flash_spi_handle, 0,
      g_context.flash_test_data, FLASH_SPI_PAGE_SIZE);
  if (status != kStatus_Success) {
    LOGE(TAG,"Flash write page cache error: %ld\n", status);
    return;
  }
#endif

}

static void
noise_test_flash_page_read()
{
  LOGE(TAG,"Noise test flash, page read is NOT yet implemented.");
}

static void
noise_test_flash_page_write()
{
  LOGE(TAG,"Noise test flash, page write is NOT yet implemented.");
}



/*****************************************************************************/
// State machine

static void
log_event(noise_test_event_t *event)
{
  switch (event->type) {
    case NOISE_TEST_EVENT_FLASH_TICK:
      // do nothing -- suppress these debug prints.
      break;
    default:
      LOGV(TAG, "[%s] Event: %s", noise_test_state_name(g_context.state), noise_test_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(noise_test_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s", noise_test_state_name(g_context.state), noise_test_event_type_name(event->type));
}

static void
set_state(noise_test_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]", noise_test_state_name(g_context.state), noise_test_state_name(state));

  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  noise_test_event_t event = { NOISE_TEST_EVENT_ENTER_STATE, (void *) state };
  handle_event(&event);
}


//
// Event handlers for the various application states:
//
static void
handle_state_idle(noise_test_event_t *event){

  switch (event->type) {
    case NOISE_TEST_EVENT_ENTER_STATE:
      break;

    case NOISE_TEST_EVENT_FLASH_START:
    {
      // save off the test cases
      if(event != NULL){
        noise_test_event_flash_start_data_t* flash_test_data = (noise_test_event_flash_start_data_t*) event->user_data;
        strcpy(g_context.flash_test_cases, flash_test_data->cases);
        set_state(NOISE_TEST_STATE_FLASH_TEST);
      }else{
        LOGE(TAG,"No argument received with flash test start event.");
      }
      break;
    }
    default:
      log_event_ignored(event);
      break;
  }

}

static void
handle_state_flash_test(noise_test_event_t *event)
{
  switch (event->type) {
    case NOISE_TEST_EVENT_ENTER_STATE:
    {
      // reset the cases index
      g_context.flash_test_cases_index = 0;
      // allocate memory for flash test
      g_context.flash_test_data = (uint8_t*) malloc(FLASH_SPI_PAGE_SIZE);
      if (g_context.flash_test_data == NULL) {
        LOGE(TAG,"Unable to allocate noise test data buffer!");
        return;
      }
      // start timer
      if( xTimerStart( g_context.flash_test_timer, portMAX_DELAY ) != pdPASS ) {
        // Sampling timer failed to start
        LOGE(TAG, "Failed to start flash test timer.");
        return;
      }
      break;
    }
    case NOISE_TEST_EVENT_FLASH_STOP:
    {
      // stop timer
      if( xTimerStop( g_context.flash_test_timer, portMAX_DELAY ) != pdPASS ) {
        // Sampling timer failed to stop
        LOGE(TAG, "Failed to stop flash test timer.");
      }
      // free memory for flash test
      free(g_context.flash_test_data);
      set_state(NOISE_TEST_STATE_IDLE);
      break;
    }
    case NOISE_TEST_EVENT_FLASH_TICK:
    {
      char c = g_context.flash_test_cases[g_context.flash_test_cases_index];
      switch(c){
      case 'r':
        noise_test_flash_cache_read();
        g_context.flash_test_cases_index++;
        break;
      case 'w':
        noise_test_flash_cache_write();
        g_context.flash_test_cases_index++;
        break;
      case 'R':
        noise_test_flash_page_read();
        g_context.flash_test_cases_index++;
        break;
      case 'W':
        noise_test_flash_page_write();
        g_context.flash_test_cases_index++;
        break;
      case '\0':
        g_context.flash_test_cases_index = 0;
        break;
      default:
        // do nothing
        break;
      }

      break;
    }
    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_event(noise_test_event_t *event)
{
  switch (g_context.state) {
    case NOISE_TEST_STATE_IDLE:
      handle_state_idle(event);
      break;

    case NOISE_TEST_STATE_FLASH_TEST:
      handle_state_flash_test(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown audio state: %d", (int) g_context.state);
      break;
  }
}


void
noise_test_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(NOISE_TEST_EVENT_QUEUE_SIZE,sizeof(noise_test_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "noise_test_event_queue");
}


static void
task_init()
{
  // Any post-scheduler init goes here.

  // Set sample period
  // 500  ms = 20Hz
  // 1000 ms = 10Hz
  g_context.flash_test_period_ticks = pdMS_TO_TICKS(500);

  g_context.flash_test_timer = xTimerCreateStatic(
      "Flash_Test_Timer",
      g_context.flash_test_period_ticks,
      pdTRUE,
      ( void * ) 0,
      flash_test_timer_cb,
      &(g_context.flash_test_timer_struct));

  set_state(NOISE_TEST_STATE_IDLE);

  LOGV(TAG, "Task launched. Entering event loop.");
}


void
noise_test_task(void *ignored)
{
  task_init();

  noise_test_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);
  }
}


/*
 * Called periodically from an xTimer to kick off a flash read/write.
 */
void flash_test_timer_cb( TimerHandle_t xTimer )
{
  noise_test_event_t event = {.type = NOISE_TEST_EVENT_FLASH_TICK, .user_data = NULL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}


#endif


