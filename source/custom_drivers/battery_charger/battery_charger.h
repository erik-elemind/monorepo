/*
 * battery_charger.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger, Tyler Gage, David Wang
 *
 * Description: BQ25887 Battery Charger driver.
 *
 * Battery Charger driver. Handles I2C communication, charge enable
 * output, and status input, as well as interrupt input.
 */
#ifndef BATTERY_CHARGER_H
#define BATTERY_CHARGER_H

#include <stdint.h>
#include <stdbool.h>

#include "i2c.h"
#include "FreeRTOS.h"
#include "timers.h"


#ifdef __cplusplus
extern "C" {
#endif

/// Battery charger status
typedef enum {
  BATTERY_CHARGER_STATUS_ON_BATTERY, ///< No input source or bad input source
  BATTERY_CHARGER_STATUS_CHARGING, ///< Good input source, charging
  BATTERY_CHARGER_STATUS_CHARGE_COMPLETE, ///< Good input source, not charging
  BATTERY_CHARGER_STATUS_FAULT ///< Problem with battery or input source
} battery_charger_status_t;


/** Initialize battery charger driver.

  ToDo Implement for REVD (not sure this will be needed)
 */
void battery_charger_init(void);

/** Enable/disable charging.

    @param enable True to enable charging, false to disable

    @return kStatus_Success if successful
 */
status_t battery_charger_enable(bool enable);

/** Get charging enabled state.

    @return True if charging is enabled, false if disabled.
 */
bool battery_charger_is_enabled(void);


/** Get charger status.

    @return Battery charger status value from enum
 */
battery_charger_status_t battery_charger_get_status();

/** Output detailed charger status.

    Prints decoded charger status registers to debug console. We keep
    this function in the battery charger driver so that we don't have
    to expose the internal register structures and constants outside
    the driver, since this is a debug-only function.

    @return kStatus_Success if successful
 */
status_t battery_charger_print_detailed_status();


#ifdef __cplusplus
}
#endif

#endif  // BATTERY_CHARGER_H
