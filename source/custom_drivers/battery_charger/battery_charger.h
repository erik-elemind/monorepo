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

/// Battery charger device handle. Must be initialized by
/// battery_charger_init().
typedef struct {
  i2c_rtos_handle_t* i2c_handle;
  uint8_t charge_enable_port;
  uint8_t charge_enable_pin;
  uint8_t status_port;
  uint8_t status_pin;
  TimerHandle_t recharge_timer_handle;
  StaticTimer_t recharge_timer_struct;
} battery_charger_handle_t;

/// Battery charger status
typedef enum {
  BATTERY_CHARGER_STATUS_ON_BATTERY, ///< No input source or bad input source
  BATTERY_CHARGER_STATUS_CHARGING, ///< Good input source, charging
  BATTERY_CHARGER_STATUS_CHARGE_COMPLETE, ///< Good input source, not charging
  BATTERY_CHARGER_STATUS_FAULT ///< Problem with battery or input source
} battery_charger_status_t;


/** Initialize battery charger driver.

    Sets up GPIOs and BQ25887 registers for the Morpheus board, and
    creates a timer to restart the charging cycle.

    Note that we don't provide an I2C address here, since address is
    not configurable for this chip.

    @param handle Handle structure to initialize
    @param i2c_handle I2C handle from I2C_RTOS_Init()
    @param charge_enable_port Port for BAT_CEn GPIO
    @param charge_enable_pin Pin for BAT_CEn GPIO
    @param status_port Port for BAT_STATn GPIO
    @param status_pin Pin for BAT_STATn GPIO
 */
void
battery_charger_init(
  battery_charger_handle_t* handle,
  i2c_rtos_handle_t* i2c_handle,
  uint8_t charge_enable_port,
  uint8_t charge_enable_pin,
  uint8_t status_port,
  uint8_t status_pin
  );

/** Enable/disable charging.

    @param handle Handle from battery_charger_init()
    @param enable True to enable charging, false to disable

    @return kStatus_Success if successful
 */
status_t
battery_charger_enable(
  battery_charger_handle_t* handle,
  bool enable
  );

/** Get charging enabled state.

    @param handle Handle from battery_charger_init()

    @return True if charging is enabled, false if disabled.
 */
bool
battery_charger_is_enabled(
  battery_charger_handle_t* handle
  );


/** Get charger status.

    @param handle Handle from battery_charger_init()

    @return Battery charger status value from enum
 */
battery_charger_status_t
battery_charger_get_status(
  battery_charger_handle_t* handle
  );

/** Output detailed charger status.

    Prints decoded charger status registers to debug console. We keep
    this function in the battery charger driver so that we don't have
    to expose the internal register structures and constants outside
    the driver, since this is a debug-only function.

    @param handle Handle from battery_charger_init()

    @return kStatus_Success if successful
 */
status_t
battery_charger_print_detailed_status(
  battery_charger_handle_t* handle
  );

/** Enable or Disable ADC.

    Enable ADC

    @param handle Handle from battery_charger_init()

    @return kStatus_Success if successful
 */
status_t
battery_charger_set_adc_enable(
  battery_charger_handle_t* handle,
  bool enable
  );

#ifdef __cplusplus
}
#endif

#endif  // BATTERY_CHARGER_H
