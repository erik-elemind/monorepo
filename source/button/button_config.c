/*
 * custom_button.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jun, 2020
 * Author:  David Wang
 *
 * Description: Provides implementation-specific initialization for
 *              the generic button task (button.h/c).
 */

#include "button_config.h"
#include "config.h"
#include "app.h"

// Example values for a hard, clean, tactile button with well-defined click:
/*
static button_timing_t button_timing1 =
{
    .debounce_delay_ticks       = pdMS_TO_TICKS(15),
    .click_timeout_ticks        = pdMS_TO_TICKS(70),
    .double_click_timeout_ticks = pdMS_TO_TICKS(600),
    .long_click_min_ticks       = pdMS_TO_TICKS(3000) // hold button for >3 seconds
};
*/

// Example values for a soft, mushy button with lots of travel and debounce noise:
static const button_timing_t button_timing2 =
{
    .debounce_delay_ticks       = pdMS_TO_TICKS(35),
    .click_timeout_ticks        = pdMS_TO_TICKS(250),
    .double_click_timeout_ticks = pdMS_TO_TICKS(600),
    .long_click_min_ticks       = pdMS_TO_TICKS(3000), // hold button for >3 seconds
    .start_repeat_ticks         = pdMS_TO_TICKS(350), // After start_repeat_ticks, ...
    .repeat_ticks               = pdMS_TO_TICKS(100), // notify every repeat_ticks when held.
};

static button_param_t button_param_arr[] =
{
#ifdef ACTIVITY_BUTTON_PORT
  {
    .port          = ACTIVITY_BUTTON_PORT,
    .pin           = ACTIVITY_BUTTON_PIN,
    .up_level      = ACTIVITY_BUTTON_UP_LEVEL,
    .cb_down       = app_event_power_button_down,
    .cb_up         = app_event_power_button_up,
    .cb_click      = app_event_power_button_click,
    .cb_double_click = app_event_power_button_double_click,
    .cb_long_click = app_event_power_button_long_click,
    .cb_held       = NULL,
    .timing        = &button_timing2,
    .state         = {ACTIVITY_BUTTON_UP_LEVEL}
  },
#endif
#ifdef VOL_UP_BUTTON_PORT
  {
    .port          = VOL_UP_BUTTON_PORT,
    .pin           = VOL_UP_BUTTON_PIN,
    .up_level      = VOL_UP_BUTTON_UP_LEVEL,
    .cb_down       = app_event_volup_button_down,
    .cb_up         = app_event_volup_button_up,
    .cb_click      = app_event_volup_button_click,
    .cb_double_click = NULL,
    .cb_long_click = NULL,
    .cb_held       = app_event_volup_button_click,
    .timing        = &button_timing2,
    .state         = {VOL_UP_BUTTON_UP_LEVEL}
  },
#endif
#ifdef VOL_DOWN_BUTTON_PORT
  {
    .port          = VOL_DOWN_BUTTON_PORT,
    .pin           = VOL_DOWN_BUTTON_PIN,
    .up_level      = VOL_DOWN_BUTTON_UP_LEVEL,
    .cb_down       = app_event_voldn_button_down,
    .cb_up         = app_event_voldn_button_up,
    .cb_click      = app_event_voldn_button_click,
    .cb_held       = app_event_voldn_button_click,
    .cb_double_click = NULL,
    .cb_long_click = NULL,
    .timing        = &button_timing2,
    .state         = {VOL_DOWN_BUTTON_UP_LEVEL}
  },
#endif
};

static button_params_t button_params =
{
  .gpio_base   = GPIO,
  .buttons     = button_param_arr,
  .num_buttons = sizeof(button_param_arr)/sizeof(button_param_t)
};

button_params_t* button_pretask_init()
{
  return &button_params;
}
