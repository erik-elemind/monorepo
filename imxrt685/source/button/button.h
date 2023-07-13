#ifndef BUTTON_H_INC
#define BUTTON_H_INC

#include "fsl_gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! @brief GINT PINT Callback function. */
typedef void (*button_cb_t)(void);

typedef struct button_timing_t
{
  TickType_t  debounce_delay_ticks;
  TickType_t  click_timeout_ticks;
  TickType_t  double_click_timeout_ticks;
  TickType_t  long_click_min_ticks;
  TickType_t  start_repeat_ticks;
  TickType_t  repeat_ticks;
} button_timing_t;

typedef struct button_state_t
{
  bool        level;
  bool        not_first_run;
  TickType_t  last_button_down_time_ticks;
  TickType_t  last_button_click_time_ticks;
  TickType_t  last_button_held_time_ticks;
} button_state_t;

typedef struct button_param_t
{
  const uint8_t   port;
  const uint8_t   pin;
  const bool      up_level;     // value when button NOT pressed, 1 or 0.
  button_cb_t     cb_up;
  button_cb_t     cb_down;
  button_cb_t     cb_click;
  button_cb_t     cb_double_click;
  button_cb_t     cb_long_click;
  // cb_held is called if button is held longer than
  // start_repeat_ticks, and is called every repeat_ticks thereafter.
  // Note that all other callbacks are still called as normal,
  // including cb_long_click.
  button_cb_t     cb_held;
  button_timing_t const *timing;      // timing struct
  button_state_t  state;        // keep track of changing button states.

} button_param_t;

typedef struct button_params_t
{
  GPIO_Type       *gpio_base;   // GPIO register pointer
  button_param_t  *buttons;     // array of button records
  uint8_t         num_buttons;  // size of button records array
} button_params_t;


// This module uses the minimalist xTaskNotifyFromISR(), which needs
// the button_task_handle from main.cpp:


void button_task(void *button_params);
void button_isr(uint32_t button_id);

#ifdef __cplusplus
}
#endif

#endif  // BUTTON_H_INC
