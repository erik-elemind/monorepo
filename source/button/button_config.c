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
#ifdef POWER_BUTTON_PORT
  {
    .port          = POWER_BUTTON_PORT,
    .pin           = POWER_BUTTON_PIN,
    .up_level      = POWER_BUTTON_UP_LEVEL,
    .cb_down       = app_event_power_button_down,
    .cb_up         = app_event_power_button_up,
    .cb_click      = app_event_power_button_click,
    .cb_double_click = app_event_power_button_double_click,
    .cb_long_click = app_event_power_button_long_click,
    .cb_held       = NULL,
    .timing        = &button_timing2,
    .state         = {0}
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
    .state         = {0}
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
    .state         = {0}
  },
#endif
};

static button_params_t button_params =
{
  .gpio_base   = GPIO,
  .buttons     = button_param_arr,
  .num_buttons = sizeof(button_param_arr)/sizeof(button_param_t)
};

void gint0_isr();

#ifdef POWER_BUTTON_PORT
void gint_pint_power_button_isr(GPIO_Type *base, uint32_t port, uint32_t pin, gint_pint_mode mode)
{
  // The button id is an index into both arrays:
  // "button_param_arr" and "gint_pint_pin_user_button_array".
  button_isr(0);
}
#endif

#ifdef VOL_UP_BUTTON_PORT
void gint_pint_vol_up_button_isr(GPIO_Type *base, uint32_t port, uint32_t pin, gint_pint_mode mode)
{
  // The button id is an index into both arrays:
  // "button_param_arr" and "gint_pint_pin_user_button_array".
  button_isr(1);
}
#endif

#ifdef VOL_DOWN_BUTTON_PORT
void gint_pint_vol_down_button_isr(GPIO_Type *base, uint32_t port, uint32_t pin, gint_pint_mode mode)
{
  // The button id is an index into both arrays:
  // "button_param_arr" and "gint_pint_pin_user_button_array".
  button_isr(2);
}
#endif

static gint_pint_pin_param_t gint_pint_pin_user_button_array[] =
{
#ifdef POWER_BUTTON_PORT
  {
    .port     = POWER_BUTTON_PORT,
    .pin      = POWER_BUTTON_PIN,
    .mode     = gint_pint_mode_change,
    .callback = gint_pint_power_button_isr
  },
#endif
#ifdef VOL_UP_BUTTON_PORT
  {
    .port     = VOL_UP_BUTTON_PORT,
    .pin      = VOL_UP_BUTTON_PIN,
    .mode     = gint_pint_mode_change,
    .callback = gint_pint_vol_up_button_isr
  },
#endif
#ifdef VOL_DOWN_BUTTON_PORT
  {
    .port     = VOL_DOWN_BUTTON_PORT,
    .pin      = VOL_DOWN_BUTTON_PIN,
    .mode     = gint_pint_mode_change,
    .callback = gint_pint_vol_down_button_isr
  },
#endif
};

static gint_pint_param_t gint_pint_user_buttons =
{
    .gpio_base          = GPIO,
//    .gint_base          = NULL, //Todo: Need to port this to new MCU
//    .gint_irq_callback  = NULL,
//    .gint_irq_n         = NULL,
    .gint_irq_priority  = NULL,
    .monitored_pins     = gint_pint_pin_user_button_array,
    .num_monitored_pins = sizeof(gint_pint_pin_user_button_array)/sizeof(gint_pint_pin_param_t)
};


void gint0_isr()
{
#if 1
  gint_pint_update(&gint_pint_user_buttons, true);
#endif
}

button_params_t* button_pretask_init()
{
  gint_pint_erase_config(&gint_pint_user_buttons);
  gint_pint_config(&gint_pint_user_buttons, true);

  return &button_params;
}
