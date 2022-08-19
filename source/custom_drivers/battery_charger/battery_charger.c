/*
 * battery_charger.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger
 *
 * Description: BQ25618 Battery Charger driver implementation.
 */
#include "battery_charger.h"
#include "battery_charger_regs.h"

#include "FreeRTOS.h"
#include "utils.h"
#include "config.h"
#include "loglevels.h"


/** Collected charger status registers.

    Note that we must read Charger Status 1 and Charger Status 2 twice
    to get the previous and current values, since the fault bits latch
    until read. */
typedef struct {
  charger_status_0_t status_0;
  charger_status_1_t status_1_previous;
  charger_status_1_t status_1_current;
  charger_status_2_t status_2_previous;
  charger_status_2_t status_2_current;
} charger_status_t;

/// BQ25618 I2C address (right-aligned)
static const uint8_t BQ25618_ADDR = 0x6A;

/// Input Current Limit register
static const uint8_t REG_INPUT_CURRENT_LIMIT = 0x00;
static const uint8_t IINDPM_2400_MA = 0b10111;
static const uint8_t IINDPM_MAX = 0b11111;

/// Charger Control 0 register
static const uint8_t REG_CHARGER_CONTROL_0 = 0x01;

/// Charger Control 1 register
static const uint8_t REG_CHARGER_CONTROL_1 = 0x05;
static const uint8_t WATCHDOG_DISABLE = 0;

/// Charge Current Limit register
static const uint8_t REG_CHARGE_CURRENT_LIMIT = 0x02;
static const uint8_t ICHG_700_MA = 0b100011;
static const uint8_t ICHG_MAX = 0b111011;

/// Charger Status 0 register
static const uint8_t REG_CHARGER_STATUS_0 = 0x08;
enum {
  CHRG_STAT_NOT_CHARGING = 0,
  CHRG_STAT_SLOW_CHARGE = 1,
  CHRG_STAT_FAST_CHARGE = 2,
  CHRG_STAT_DONE_CHARGING = 3
};

/// Charger Status 1 register
static const uint8_t REG_CHARGER_STATUS_1 = 0x09;

/// Charger Status 2 register
static const uint8_t REG_CHARGER_STATUS_2 = 0x0A;
static const uint8_t CHARGER_STATUS_2_FAULTS = 0x70;

/// Part Information register
static const uint8_t REG_PART_INFORMATION = 0x0B;

// Rate to tickle watchdog
static const int MS_PER_S = 1000;
static const int WATCHDOG_TICKLE_MS = (20 * MS_PER_S);

/// Logging prefix
static const char* TAG = "battery_charger";


// Local function declarations

/** Reset all battery charger registers.

    @param handle Handle from battery_charger_init()
 */
static status_t
battery_charger_reset_device(battery_charger_handle_t* handle);

/** Reset battery charger watchdog.

    Required for charging (if watchdog timer is enabled). Extracted
    into its own function so that it can easily be called both from
    initialization code and from timer handler.

    @param handle Handle from battery_charger_init()
 */
static status_t
battery_charger_reset_watchdog(battery_charger_handle_t* handle);

/** Handle watchdog timer.

    @param timer_handle FreeRTOS timer handle from xTimerCreate
 */
static void
battery_charger_watchdog_timer(TimerHandle_t timer_handle);

/** Read charger status registers.

    @param handle Handle from battery_charger_init()
    @param p_charger_status Pointer to struct to receive status register data
 */
static status_t
battery_charger_read_status_registers(
  battery_charger_handle_t* handle,
  charger_status_t* p_charger_status
  );

/** Set input current limit.

    Sets the input current in the Input Current Limit register. This
    is reset by the charger every time an input source is detected and
    tested, so it should be updated when a transition from battery to
    an external input source is detected.

    Note that the iindpm value provided is not in mA--it's written
    directly to the register. See datasheet for values.

    @param handle Handle from battery_charger_init()
    @param iindpm Value for for IINDPM in Input Current Limit register
 */
static status_t
battery_charger_set_input_current(
  battery_charger_handle_t* handle,
  uint8_t iindpm
  );

/** Enables monitoring of the temperature sensor

    Sets whether to monitor the temperature sensor in the
    Input Current Limit register. This
    is reset by the charger every time an input source is detected and
    tested, so it should be updated when a transition from battery to
    an external input source is detected.

    @param handle Handle from battery_charger_init()
    @param ignore true to ignore the temperature sensor, false to use it.
 */

static status_t
battery_charger_set_ts_ignore(
  battery_charger_handle_t* handle,
  bool ignore
  );

/** Set charge current limit.

    Sets the "fast charge" current in the Charge Current Limit
    register.

    Note that the ichg value provided is not in mA--it's written
    directly to the register. See datasheet for values.

    @param handle Handle from battery_charger_init()
    @param ichg Value for for ICHG in Charge Current Limit register
 */
static status_t
battery_charger_set_charge_current(
  battery_charger_handle_t* handle,
  uint8_t ichg
  );


// Global function definitions

void
battery_charger_init(
  battery_charger_handle_t* handle,
  i2c_rtos_handle_t* i2c_handle,
  uint8_t charge_enable_port,
  uint8_t charge_enable_pin,
  uint8_t status_port,
  uint8_t status_pin
  )
{
  handle->i2c_handle = i2c_handle;
  handle->charge_enable_port = charge_enable_port;
  handle->charge_enable_pin = charge_enable_pin;
  handle->status_port = status_port;
  handle->status_pin = status_pin;

  /* Note that GPIO ports must be initialized with GPIO_PortInit()
     before this is called. */
  GPIO_PinInit(GPIO, handle->charge_enable_port, handle->charge_enable_pin,
      &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});
  GPIO_PinInit(GPIO, handle->status_port, handle->status_pin,
      &(gpio_pin_config_t){kGPIO_DigitalInput, 0});

  /* Reset device. Since the BQ25618 is powered as long as the
     battery has some charge, the registers will never get reset in
     normal operation unless we explicitly reset the device here. */
  status_t status = battery_charger_reset_device(handle);
  if (status != kStatus_Success) {
    LOGE(TAG, "Error resetting device: %ld", status);
  }

  // Create repeating timer to tickle watchdog
  handle->watchdog_timer_handle = xTimerCreateStatic("BATTERY_CHARGER",
    pdMS_TO_TICKS(WATCHDOG_TICKLE_MS), pdTRUE, handle,
    battery_charger_watchdog_timer, &(handle->watchdog_timer_struct));
  if (xTimerStart(handle->watchdog_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to start watchdog timer!");
  }

  // Tickle watchdog now
  status = battery_charger_reset_watchdog(handle);
  if (status != kStatus_Success) {
    LOGE(TAG, "Error resetting watchdog: %ld", status);
  }

  // Read status registers twice to verify watchdog reset
  charger_status_t charger_status;
  status = battery_charger_read_status_registers(handle, &charger_status);
  if (status == kStatus_Success) {
    LOGV(TAG, "Status 0: 0x%x", charger_status.status_0.raw);
    LOGV(TAG, "Status 1 read 1: 0x%x", charger_status.status_1_previous.raw);
    LOGV(TAG, "Status 1 read 2: 0x%x", charger_status.status_1_current.raw);
    LOGV(TAG, "Status 2 read 1: 0x%x", charger_status.status_2_previous.raw);
    LOGV(TAG, "Status 2 read 2: 0x%x", charger_status.status_2_current.raw);
  }
  else {
    LOGE(TAG, "Error reading status registers: %ld", status);
  }

  // Set input current limit to 2400 mA
  status = battery_charger_set_input_current(handle, IINDPM_2400_MA);
  if (status != kStatus_Success) {
    LOGE(TAG, "Error setting input current: %ld", status);
  }

  // TODO: Do NOT ignore the temperature sensor in production
  // Ignore the temperature sensor

#if (defined(ENABLE_CHARGER_TEMP_SENSOR) && (ENABLE_CHARGER_TEMP_SENSOR > 0U))
  status = battery_charger_set_ts_ignore(handle, false);
#else
  status = battery_charger_set_ts_ignore(handle, true);
#endif
  if (status != kStatus_Success) {
    LOGE(TAG, "Error setting ignore temperature sensor: %ld", status);
  }

  // Set charge current limit to 700 mA
  status = battery_charger_set_charge_current(handle, ICHG_700_MA);
  if (status != kStatus_Success) {
    LOGE(TAG, "Error setting charge current: %ld", status);
  }
}

status_t
battery_charger_enable(
  battery_charger_handle_t* handle,
  bool enable
  )
{
  GPIO_PinWrite(GPIO,
    handle->charge_enable_port, handle->charge_enable_pin, !enable);

  return kStatus_Success;
}

bool
battery_charger_is_enabled(
  battery_charger_handle_t* handle
  )
{
  return !GPIO_PinRead(GPIO,
    handle->charge_enable_port, handle->charge_enable_pin);
}

status_t
battery_charger_disable_wdog(
  battery_charger_handle_t* handle
  )
{
  charger_control_1_t charger_control_1 = { .raw = 0 };
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
    REG_CHARGER_CONTROL_1, &charger_control_1.raw);

  if (status == kStatus_Success) {
    charger_control_1.watchdog = WATCHDOG_DISABLE;
    status = i2c_mem_write_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_CHARGER_CONTROL_1, charger_control_1.raw);
  }

  return status;
}

battery_charger_status_t
battery_charger_get_status(
  battery_charger_handle_t* handle
  )
{
  battery_charger_status_t battery_charger_status =
    BATTERY_CHARGER_STATUS_FAULT;

  // Read charger status registers
  charger_status_t charger_status;
  status_t status = battery_charger_read_status_registers(handle,
    &charger_status);

  if (status == kStatus_Success) {
    /* First, check for faults.  Any bit set in Charger Status 1 is a
     fault, but only bits 6:4 in Charger Status 2 are faults. */
    /** @todo BH - Not sure if we want to consider VINDPM/IINDPM bits
        in Charger Status 2 as faults; the VINDPM_STAT bit (at least)
        will be set if the input source can't supply all 700+ mA we
        want to charge, but the battery will still charge, just
        slower. */
    if ((charger_status.status_1_current.raw != 0) ||
      ((charger_status.status_2_current.raw & CHARGER_STATUS_2_FAULTS) != 0)) {
      battery_charger_status = BATTERY_CHARGER_STATUS_FAULT;
    }
    else {
      // No faults--check for a good input source
      /// @todo bh vbus_gd or pg_stat?
      if (charger_status.status_0.pg_stat) {
        // Input source is good, check whether we're charging
        if ((charger_status.status_0.chrg_stat == CHRG_STAT_SLOW_CHARGE) ||
          (charger_status.status_0.chrg_stat == CHRG_STAT_FAST_CHARGE)) {
          // We're charging
          battery_charger_status = BATTERY_CHARGER_STATUS_CHARGING;
        }
        else {
          // We're not charging, but input source is good and no faults
          battery_charger_status = BATTERY_CHARGER_STATUS_CHARGE_COMPLETE;
        }
      }
      else {
        // No good input source, but no faults
        battery_charger_status = BATTERY_CHARGER_STATUS_ON_BATTERY;
      }
    }
  }
  else {
    // Couldn't read one of the Charger Status registers--return fault
    battery_charger_status = BATTERY_CHARGER_STATUS_FAULT;

  }

  return battery_charger_status;
}

status_t
battery_charger_print_detailed_status(
  battery_charger_handle_t* handle
  )
{
  /* Note that we use printf() instead of LOGx() here, since this is
     an extension of a shell command (implemented here so that we
     don't have to expose internal state). Shell command output always
     uses printf() to output directly to the console, while log
     messages use the LOGx() functions so that we can redirect the log
     messages (i.e. to a file) if neccessary). */

  printf("  Charging: %s\n",
    battery_charger_is_enabled(handle) ? "ENABLED" : "DISABLED");

  charger_status_t charger_status;
  status_t status = battery_charger_read_status_registers(handle,
    &charger_status);
  if (status == kStatus_Success) {
    printf("  Charger Status 0 (REG08): 0x%x\n", charger_status.status_0.raw);
    printf("    vsys_stat: %d\n", charger_status.status_0.vsys_stat);
    printf("    therm_stat: %d\n", charger_status.status_0.therm_stat);
    printf("    pg_stat: %d\n", charger_status.status_0.pg_stat);
    printf("    chrg_stat: %d\n", charger_status.status_0.chrg_stat);
    printf("    vbus_stat: %d\n", charger_status.status_0.vbus_stat);

    printf("  Charger Status 1 (REG09), latched values: 0x%x\n",
      charger_status.status_1_previous.raw);
    printf("    ntc_fault: %d\n",
      charger_status.status_1_previous.ntc_fault);
    printf("    bat_fault: %d\n",
      charger_status.status_1_previous.bat_fault);
    printf("    chrg_fault: %d\n",
      charger_status.status_1_previous.chrg_fault);
    printf("    boost_fault: %d\n",
      charger_status.status_1_previous.boost_fault);
    printf("    watchdog_fault: %d\n",
      charger_status.status_1_previous.watchdog_fault);

    if (charger_status.status_1_previous.raw ==
      charger_status.status_1_current.raw) {
      printf("  Charger Status 1 (REG09), current values: same as latched\n");
    }
    else {
      printf("  Charger Status 1 (REG09), current values: 0x%x\n",
        charger_status.status_1_current.raw);
      printf("    ntc_fault: %d\n",
        charger_status.status_1_current.ntc_fault);
      printf("    bat_fault: %d\n",
        charger_status.status_1_current.bat_fault);
      printf("    chrg_fault: %d\n",
        charger_status.status_1_current.chrg_fault);
      printf("    boost_fault: %d\n",
        charger_status.status_1_current.boost_fault);
      printf("    watchdog_fault: %d\n",
        charger_status.status_1_current.watchdog_fault);
    }

    printf("  Charger Status 2 (REG0A), latched values: 0x%x\n",
      charger_status.status_2_previous.raw);
    printf("    iindpm_int_mask: %d\n",
      charger_status.status_2_previous.iindpm_int_mask);
    printf("    windpm_int_mask: %d\n",
      charger_status.status_2_previous.windpm_int_mask);
    printf("    acov_stat: %d\n",
      charger_status.status_2_previous.acov_stat);
    printf("    topoff_active: %d\n",
      charger_status.status_2_previous.topoff_active);
    printf("    batsns_stat: %d\n",
      charger_status.status_2_previous.batsns_stat);
    printf("    iintpm_stat: %d\n",
      charger_status.status_2_previous.iintpm_stat);
    printf("    vindpm_stat: %d\n",
      charger_status.status_2_previous.vindpm_stat);
    printf("    vbus_gd: %d\n",
      charger_status.status_2_previous.vbus_gd);

    if (charger_status.status_2_previous.raw ==
      charger_status.status_2_current.raw) {
      printf("  Charger Status 2 (REG0A), current values: same as latched\n");
    }
    else {
      printf("  Charger Status 2 (REG0A), current values: 0x%x\n",
        charger_status.status_2_current.raw);
      printf("    iindpm_int_mask: %d\n",
        charger_status.status_2_current.iindpm_int_mask);
      printf("    windpm_int_mask: %d\n",
        charger_status.status_2_current.windpm_int_mask);
      printf("    acov_stat: %d\n",
        charger_status.status_2_current.acov_stat);
      printf("    topoff_active: %d\n",
        charger_status.status_2_current.topoff_active);
      printf("    batsns_stat: %d\n",
        charger_status.status_2_current.batsns_stat);
      printf("    iintpm_stat: %d\n",
        charger_status.status_2_current.iintpm_stat);
      printf("    vindpm_stat: %d\n",
        charger_status.status_2_current.vindpm_stat);
      printf("    vbus_gd: %d\n",
        charger_status.status_2_current.vbus_gd);
    }
  }

  return status;
}

// Local function definitions

static status_t
battery_charger_reset_device(battery_charger_handle_t* handle)
{
  part_information_t part_information = { .reg_rst = true };
  status_t status = i2c_mem_write_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_PART_INFORMATION, part_information.raw);

  return status;
}

static status_t
battery_charger_reset_watchdog(battery_charger_handle_t* handle)
{
  charger_control_0_t charger_control_0;
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
    REG_CHARGER_CONTROL_0, &charger_control_0.raw);
  if (status == kStatus_Success) {
    charger_control_0.wd_rst = true;
    status = i2c_mem_write_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_CHARGER_CONTROL_0, charger_control_0.raw);
    if (status == kStatus_Success) {
      /// @todo BH - Can't log from SW timer, apparently?
      //LOGV(TAG, "Watchdog timer successfully reset");
    }
    else {
      /*LOGE(TAG, "Watchdog: Error writing Charger Control 0 register: %ld",
        status);*/
    }
  }
  else {
    /*LOGE(TAG, "Watchdog: Error reading Charger Control 0 register: %ld",
      status);*/
  }

  return status;
}

static void
battery_charger_watchdog_timer(TimerHandle_t timer_handle)
{
  battery_charger_handle_t* handle =
    (battery_charger_handle_t*)pvTimerGetTimerID(timer_handle);
  battery_charger_reset_watchdog(handle);
}

static status_t
battery_charger_read_status_registers(
  battery_charger_handle_t* handle,
  charger_status_t* p_charger_status
  )
{
  /* Read status registers.

     Read status_1 and status_2 registers twice, to get both previous
     and current status values. */

  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
    REG_CHARGER_STATUS_0, &p_charger_status->status_0.raw);

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_CHARGER_STATUS_1, &p_charger_status->status_1_previous.raw);
  }

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_CHARGER_STATUS_1, &p_charger_status->status_1_current.raw);
  }

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_CHARGER_STATUS_2, &p_charger_status->status_2_previous.raw);
  }

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_CHARGER_STATUS_2, &p_charger_status->status_2_current.raw);
  }

  return status;
}

static status_t
battery_charger_set_input_current(
  battery_charger_handle_t* handle,
  uint8_t iindpm
  )
{
  if (iindpm > IINDPM_MAX) {
    LOGE(TAG, "Error: Invalid IINDPM value: 0x%x > 0x%x", iindpm, IINDPM_MAX);
    return kStatus_Fail;
  }

  input_current_limit_t input_current_limit;
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
    REG_INPUT_CURRENT_LIMIT, &input_current_limit.raw);
  if (status == kStatus_Success) {
    input_current_limit.iindpm = iindpm;
    status = i2c_mem_write_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_INPUT_CURRENT_LIMIT, input_current_limit.raw);
    if (status != kStatus_Success) {
      LOGE(TAG, "Error writing Input Current Limit register: %ld", status);
    }
  }
  else {
    LOGE(TAG, "Error reading Input Current Limit register: %ld", status);
  }

  return status;
}

static status_t
battery_charger_set_ts_ignore(
  battery_charger_handle_t* handle,
  bool ignore
  )
{

  input_current_limit_t input_current_limit;
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
    REG_INPUT_CURRENT_LIMIT, &input_current_limit.raw);
  if (status == kStatus_Success) {
    input_current_limit.ts_ignore = ignore;
    status = i2c_mem_write_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_INPUT_CURRENT_LIMIT, input_current_limit.raw);
    if (status != kStatus_Success) {
      LOGE(TAG, "Error writing Input Current Limit register: %ld", status);
    }
  }
  else {
    LOGE(TAG, "Error reading Input Current Limit register: %ld", status);
  }

  return status;
}

static status_t
battery_charger_set_charge_current(
  battery_charger_handle_t* handle,
  uint8_t ichg
  )
{
  if (ichg > ICHG_MAX) {
    LOGE(TAG, "Error: Invalid ICHG value: 0x%x > 0x%x", ichg, ICHG_MAX);
    return kStatus_Fail;
  }

  charger_current_limit_t charge_current_limit;
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25618_ADDR,
    REG_CHARGE_CURRENT_LIMIT, &charge_current_limit.raw);
  if (status == kStatus_Success) {
    charge_current_limit.ichg = ichg;
    status = i2c_mem_write_byte(handle->i2c_handle, BQ25618_ADDR,
      REG_CHARGE_CURRENT_LIMIT, charge_current_limit.raw);
    if (status != kStatus_Success) {
      LOGE(TAG, "Error writing Charge Current Limit register: %ld", status);
    }
  }
  else {
    LOGE(TAG, "Error reading Charge Current Limit register: %ld", status);
  }

  return status;
}
