/*
 * app.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: August, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Morpheus Application task.
 */
#ifndef APP_H
#define APP_H

#include "interpreter.h"

#ifdef __cplusplus
extern "C" {
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void app_pretask_init(void);

void app_task(void *ignored);

void app_event_power_button_down(void);
void app_event_power_button_up(void);
void app_event_power_button_click(void);
void app_event_power_button_double_click(void);
void app_event_power_button_long_click(void);
void app_event_volup_button_down(void);
void app_event_volup_button_up(void);
void app_event_volup_button_click(void);
void app_event_voldn_button_down(void);
void app_event_voldn_button_up(void);
void app_event_voldn_button_click(void);
void app_event_ble_therapy_start(therapy_type_t therapy);
void app_event_ble_activity(void);
void app_event_button_activity(void);
void app_event_shell_activity(void);
void app_event_sleep_timeout(void);
void app_event_ble_off_timeout(void);
void app_event_charger_plugged(void);
void app_event_charger_unplugged(void);
void app_event_charge_complete(void);
void app_event_charge_fault(void);

#ifdef __cplusplus
}
#endif

#endif  // APP_H
