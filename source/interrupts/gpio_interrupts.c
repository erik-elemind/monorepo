
#include "FreeRTOS.h"
#include "task.h"
#include "config.h"
#include "utils.h"

#include "system_monitor.h"
#include "config_tracealyzer_isr.h"

// TODO: Global GPIO ISRs go in this file.

#define TAG "gint"

/*
 * Pin change interrupt callback for pin PIO1_1,
 * connected to ambient light sensor.
 */

void als_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
{
    /* Take action for pin interrupt */
}

/*
 * Pin change interrupt callback for pin PIO1_3,
 * connected to battery charger.
 */
void charger_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
{
  system_monitor_event_battery_from_isr();
}

void mems_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status) {
  system_monitor_event_mic_from_isr();
}

void user_button1_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
{
  // ToDo: Implement
}

void user_button2_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
{
	// ToDo: Implement
}