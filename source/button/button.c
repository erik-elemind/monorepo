#include <stdint.h>

#include "app.h"
#include "loglevels.h"
#include "button.h"
#include "timers.h"

static const char *TAG = "button";  // Logging prefix for this module

TaskHandle_t g_button_task_handle = NULL;

#define BUTTON_TIMER_TIME_MS 100

typedef struct {
  TimerHandle_t button_timer_handle;
  StaticTimer_t button_timer_struct;
  int32_t button_timers;
} button_context_t;

static button_context_t g_context;

void call_button_cb(button_cb_t cb)
{
  if(cb){
    cb();
  }
}

#if 0
bool button_down(button_param_t *bparam)
{
  bool result = false;

  taskENTER_CRITICAL();  // to read button_state

  if (bparam->state.level != bparam->up_level) {
    result = true;
  }

  taskEXIT_CRITICAL();

  return result;
}
#endif

/*
 * Updates the state of each button,
 * computes the type of 'click', and
 * calls the button callbacks.
 */
static void button_handler(button_param_t *bparam)
{
  button_state_t        *bs = &(bparam->state);
  const button_timing_t *bt = bparam->timing;

  // TODO:
  // Need to think a bit more about how to de-bounce a button
  // in this case, where the button logic appears in one task.

  // Wait a bit before sampling, to debounce noise:
//  vTaskDelay(BUTTON_DEBOUNCE_DELAY_MS / portTICK_PERIOD_MS);

  int sampled_button_level = GPIO_PinRead(GPIO, bparam->port, bparam->pin);

  if(bparam->state.not_first_run){
    // Did we miss a toggle event?
    if (sampled_button_level == bparam->state.level) {
      LOGV(TAG, "debounce skip: %d", sampled_button_level );
      return;
    }
  }
  else {
    // This is the first run (first ISR)
    // record that this is NOT the first run.
    bparam->state.not_first_run = true;

    /* If the first event is a button-up event, ignore it--the
       button-down event happened while we were powered off, so don't
       register this as a click. */
    if (sampled_button_level == bparam->up_level) {
      LOGV(TAG, "Ignoring button up as first event");
      bs->level = bparam->up_level;
      return;
    }
    else {
      // make sure we see the state change
      bs->level = !sampled_button_level;
    }
  }

  TickType_t current_time_ticks = xTaskGetTickCount();

  if(bs->level == bparam->up_level){
    // button was just pushed DOWN
    bs->level = !bparam->up_level;
    bs->last_button_down_time_ticks = current_time_ticks;
    bs->last_button_held_time_ticks = current_time_ticks;
    call_button_cb(bparam->cb_down);

    if (bparam->cb_held) {
      if (g_context.button_timers == 0) {
        xTimerStart(g_context.button_timer_handle, 0);
      }
      g_context.button_timers++;
    }
  }
  else {
    // button was just released UP
    bs->level = bparam->up_level;
    call_button_cb(bparam->cb_up);

    g_context.button_timers--;
    if (g_context.button_timers == 0) {
      xTimerStop(g_context.button_timer_handle, 0);
    }

    TickType_t button_down_delta_ticks =
      current_time_ticks - bs->last_button_down_time_ticks;

    if (button_down_delta_ticks < bt->click_timeout_ticks) {
      // button was CLICKED
      call_button_cb(bparam->cb_click);

      TickType_t button_click_delta_ticks =
        current_time_ticks - bs->last_button_click_time_ticks;

      if (button_click_delta_ticks < bt->double_click_timeout_ticks) {
        // button was DOUBLE CLICKED
        call_button_cb(bparam->cb_double_click);
      }

      bs->last_button_click_time_ticks = current_time_ticks;
    }
    else if(button_down_delta_ticks > bt->long_click_min_ticks ){
      // button was LONG CLICKED
      call_button_cb(bparam->cb_long_click);
    }
  }

  // Tell app there was activity
  app_event_button_activity();
}

// If button is depressed and has a 'held' handler,
// check if it is time to fire the repeat handler.
static void button_held_handler(button_param_t *bparam) {
  button_state_t        *bs = &(bparam->state);
  const button_timing_t *bt = bparam->timing;

  if (!bparam->state.not_first_run) {
    return;
  }

  if (bs->level == bparam->up_level) {
    return;
  }

  if (bparam->cb_held == NULL) {
    return;
  }
  
  TickType_t current_time_ticks = xTaskGetTickCount();
  
  TickType_t button_down_delta_ticks =
    current_time_ticks - bs->last_button_down_time_ticks;

  if (button_down_delta_ticks >= bt->start_repeat_ticks) {
    TickType_t button_held_delta_ticks =
      current_time_ticks - bs->last_button_held_time_ticks;
    if (button_held_delta_ticks >= bt->repeat_ticks) {
      call_button_cb(bparam->cb_held);
      bs->last_button_held_time_ticks = current_time_ticks;
      
      // Tell app there was activity
      app_event_button_activity();
    }
  }
}


static void
button_held_timeout(TimerHandle_t timer_handle){
  // Wake up the button task to check for held events.
  xTaskNotify(g_button_task_handle,
	      0,
	      eSetBits);
}

void button_task(void *button_params)
{
  g_button_task_handle = xTaskGetCurrentTaskHandle();

  button_params_t *bparams = (button_params_t*) button_params;
  uint32_t notify_bits;

  g_context.button_timer_handle = xTimerCreateStatic("BUTTON_TIMER", pdMS_TO_TICKS(BUTTON_TIMER_TIME_MS),
						     pdTRUE, NULL, button_held_timeout,
						     &(g_context.button_timer_struct));
  
  while(1) {

    // Wait for an interrupt
    xTaskNotifyWait( pdFALSE,        /* Clear bits on entry. */
                     UINT32_MAX,     /* Clear all bits on exit. */
                     &notify_bits,   /* Stores the notified value. */
                     portMAX_DELAY );

    // Loop over each button and see if it was notified.
    for(int b=0; b < bparams->num_buttons; b++){
      uint32_t button_bit = (1<<b);
      // Check the notify bits to see if the current button caused the ISR
      if( ( notify_bits & button_bit ) != 0 ) {
        // Call the button handler
        button_handler(&(bparams->buttons[b]));
      }
    }

    // Check for held buttons
    if (g_context.button_timers) {
      for(int b=0; b < bparams->num_buttons; b++){
	button_held_handler(&(bparams->buttons[b]));
      }
    }

  }  // while(1)
}


void button_isr(uint32_t button_id)
{
  /* Check that task has been initialized and scheduler started,
     since we can get an interrupt (like power button up) before
     that. */
  if (g_button_task_handle && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Button interrupt:
    xTaskNotifyFromISR( g_button_task_handle,
        (1 << button_id),
        eSetBits,
        &xHigherPriorityTaskWoken );
    // Always do this when calling a FreeRTOS "...FromISR()" function:
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
  }

}
