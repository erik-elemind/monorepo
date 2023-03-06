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
#include "battery_charger_regs_BQ25887.h"

#include "FreeRTOS.h"
#include "utils.h"
#include "config.h"
#include "loglevels.h"

#define CHARGER_STATUS_1_FAULTS 0x70
#define NTS_STATUS_COLD 0x05
#define NTS_STATUS_HOT 0x06


/** Collected charger status registers.

    Note that we must read Charger Status 1 and Charger Status 2 twice
    to get the previous and current values, since the fault bits latch
    until read. */
typedef struct {
  charger_status_1_t charger_status_1;
  charger_status_2_t charger_status_2;
  ntc_status_t ntc_status;
  fault_status_t fault_status;
} charger_status_t;

typedef struct {
  charger_control_1_t charger_control_1;
  charger_control_3_t charger_control_3;
} charger_control_t;


typedef struct {
  adc_control_t adc_control;
  vbus_adc_1_t vbus_adc_1;
  vbus_adc_0_t vbus_adc_0;
  vbat_adc_1_t vbat_adc_1;
  vbat_adc_0_t vbat_adc_0;
  vcelltop_adc_1_t vcelltop_adc_1;
  vcelltop_adc_0_t vcelltop_adc_0;
  vcellbot_adc_1_t vcellbot_adc_1;
  vcellbot_adc_0_t vcellbot_adc_0;
} charger_adc_t;


/// BQ25887 I2C address (right-aligned)
static const uint8_t BQ25887_ADDR = 0x6A;

/// Charger Control 1 register
static const uint8_t REG_CHARGER_CONTROL_1 = 0x05;

/// Charger Control 3 register
static const uint8_t REG_CHARGER_CONTROL_3 = 0x07;

/// Charger Status 1 register
static const uint8_t REG_CHARGER_STATUS_1 = 0x0B;

/// Charger Status 2 register
static const uint8_t REG_CHARGER_STATUS_2 = 0x0C;

/// NTC Status register
static const uint8_t REG_NTC_STATUS = 0x0D;

/// Fault Status register
static const uint8_t REG_FAULT_STATUS = 0x0D;

/// ADC Control register
static const uint8_t REG_ADC_CONTROL = 0x15;

/// VBUS ADC 1 register
static const uint8_t REG_VBUS_ADC_1 = 0x1B;

/// VBUS ADC 0 register
static const uint8_t REG_VBUS_ADC_0 = 0x1C;

/// VBAT ADC 1 register
static const uint8_t REG_VBAT_ADC_1 = 0x1D;

/// VBAT ADC 0 register
static const uint8_t REG_VBAT_ADC_0 = 0x1E;

/// VCELLTOP ADC 1 register
static const uint8_t REG_VCELLTOP_ADC_1 = 0x1F;

/// VCELLTOP ADC 0 register
static const uint8_t REG_VCELLTOP_ADC_0 = 0x20;

/// VCELLBOT ADC 1 register
static const uint8_t REG_VCELLBOT_ADC_1 = 0x26;

/// VCELLBOT ADC 0 register
static const uint8_t REG_VCELLBOT_ADC_0 = 0x27;

/// Part Information register
static const uint8_t REG_PART_INFORMATION = 0x25;

// Rate to tickle watchdog
static const int MS_PER_S = 1000;
static const int WATCHDOG_TICKLE_MS = (20 * MS_PER_S);

enum {
  CHRG_STAT_NOT_CHARGING = 0,
  CHRG_STAT_TRICKLE_CHARGE = 1,
  CHRG_STAT_PRE_CHARGE = 2,
  CHRG_STAT_FAST_CHARGE = 3,
  CHRG_STAT_TAPER_CHARGE = 4,
  CHRG_STAT_TOPOFF_CHARGE = 5,
  CHRG_STAT_DONE_CHARGING = 6,
  CHRG_STAT_RESERVED = 7,
};

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

static status_t
battery_charger_read_control_registers(
  battery_charger_handle_t* handle,
  charger_control_t* p_charger_control
  );

static status_t
battery_charger_read_adc_registers(
  battery_charger_handle_t* handle,
  charger_adc_t* p_charger_adc
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

  // Disable WDT
  battery_charger_disable_wdog(handle);

  // Read status registers twice to verify watchdog reset
  charger_status_t charger_status;
  status = battery_charger_read_status_registers(handle, &charger_status);
  if (status == kStatus_Success) {
    LOGV(TAG, "Status 1: 0x%x", charger_status.charger_status_1.raw);
    LOGV(TAG, "Status 2: 0x%x", charger_status.charger_status_2.raw);
    LOGV(TAG, "Status NTC: 0x%x", charger_status.ntc_status.raw);
    LOGV(TAG, "Status FAULT: 0x%x", charger_status.fault_status.raw);
  }
  else {
    LOGE(TAG, "Error reading status registers: %ld", status);
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
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
    REG_CHARGER_CONTROL_1, &charger_control_1.raw);

  if (status == kStatus_Success) {
	  charger_control_1.watchdog = 0x00;
    status = i2c_mem_write_byte(handle->i2c_handle, BQ25887_ADDR,
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
    if ((charger_status.fault_status.raw != 0) ||
      ((charger_status.charger_status_1.raw & CHARGER_STATUS_1_FAULTS) != 0) ||
	  (charger_status.ntc_status.raw == NTS_STATUS_COLD) ||
	  (charger_status.ntc_status.raw == NTS_STATUS_HOT)) {
      battery_charger_status = BATTERY_CHARGER_STATUS_FAULT;
    }
    else {
      // No faults--check for a good input source
      /// @todo bh vbus_gd or pg_stat?
      if (charger_status.charger_status_2.pg_stat) {
        // Input source is good, check whether we're charging
        if ((charger_status.charger_status_1.chrg_status == CHRG_STAT_TRICKLE_CHARGE) ||
			(charger_status.charger_status_1.chrg_status == CHRG_STAT_PRE_CHARGE) ||
			(charger_status.charger_status_1.chrg_status == CHRG_STAT_FAST_CHARGE) ||
			(charger_status.charger_status_1.chrg_status == CHRG_STAT_TAPER_CHARGE) ||
			(charger_status.charger_status_1.chrg_status == CHRG_STAT_TOPOFF_CHARGE )) {
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
    printf("  Charger Status 1 (REG 0x0B): 0x%x\n", charger_status.charger_status_1.raw);
    printf("    chrg_stat: %d\n", charger_status.charger_status_1.chrg_status);
    printf("    wd_stat: %d\n", charger_status.charger_status_1.wd_stat);
    printf("    treg_stat: %d\n", charger_status.charger_status_1.treg_stat);
    printf("    vindpm_stat: %d\n", charger_status.charger_status_1.vindpm_stat);
    printf("    iindpm_stat: %d\n", charger_status.charger_status_1.iindpm_stat);

    printf("  Charger Status 2 (REG 0x0C): 0x%x\n",charger_status.charger_status_2.raw);
    printf("    ico_stat: %d\n", charger_status.charger_status_2.ico_stat);
	printf("    vbus_stat: %d\n", charger_status.charger_status_2.vbus_stat);
    printf("    pg_stat: %d\n", charger_status.charger_status_2.pg_stat);

    printf("  NTC Status (REG 0x0D): 0x%x\n",charger_status.ntc_status.raw);

    printf("  Fault Status (REG 0x0E): 0x%x\n",charger_status.fault_status.raw);
    printf("    tmr_stat: %d\n", charger_status.fault_status.tmr_stat);
    printf("    tshut_stat: %d\n", charger_status.fault_status.tshut_stat);
    printf("    vbus_ovp_stat: %d\n", charger_status.fault_status.vbus_ovp_stat);
  }

  charger_control_t charger_control;
    status = battery_charger_read_control_registers(handle,
      &charger_control);
    if (status == kStatus_Success) {
      printf("  Charger Control 1 (REG 0x05): 0x%x\n", charger_control.charger_control_1.raw);
      printf("    Termination Control: %d\n", charger_control.charger_control_1.en_term);
      printf("    STAT Pin Disable: %d\n", charger_control.charger_control_1.stat_dis);
      printf("    WDT Settings: %d\n", charger_control.charger_control_1.watchdog);
      printf("    Charging Safety Timer Enable: %d\n", charger_control.charger_control_1.en_timer);
      printf("    Fast Charge Timer Settings: %d\n", charger_control.charger_control_1.chg_timer);
      printf("    Safety Timer during DPM/TREG: %d\n", charger_control.charger_control_1.tmr2x_en);

      printf("  Charger Control 3 (REG 0x07): 0x%x\n",charger_control.charger_control_3.raw);
      printf("    PFM Mode Disable Control: %d\n", charger_control.charger_control_3.pfm_dis);
  	  printf("    WDT Reset: %d\n", charger_control.charger_control_3.wd_rst);
      printf("    Top off Timer Control: %d\n", charger_control.charger_control_3.topoff_timer);
    }

   charger_adc_t charger_adc;
   status = battery_charger_read_adc_registers(handle,
         &charger_adc);

   if (status == kStatus_Success) {
	   int16_t v_print=0;

        printf("  ADC Control (REG 0x15): 0x%x\n", charger_adc.adc_control.raw);
        printf("    ADC Enable: %d\n", charger_adc.adc_control.adc_en);
        printf("    ADC Rate: %d\n", charger_adc.adc_control.adc_rate);
        printf("    ADC Sample: %d\n", charger_adc.adc_control.adc_sample);

        printf("  VBUS ADC 1 (REG 0x1B): 0x%x\n", charger_adc.vbus_adc_1.raw);
        printf("  VBUS ADC 0 (REG 0x1C): 0x%x\n", charger_adc.vbus_adc_0.raw);

        printf("  VBAT ADC 1 (REG 0x1D): 0x%x\n", charger_adc.vbat_adc_1.raw);
		printf("  VBAT ADC 0 (REG 0x1E): 0x%x\n", charger_adc.vbat_adc_0.raw);

		printf("  VCELLTOP ADC 1 (REG 0x1F): 0x%x\n", charger_adc.vcelltop_adc_1.raw);
		printf("  VCELLTOP ADC 0 (REG 0x20): 0x%x\n", charger_adc.vcelltop_adc_0.raw);

		printf("  VCELLBOT ADC 1 (REG 0x1F): 0x%x\n", charger_adc.vcellbot_adc_1.raw);
		printf("  VCELLBOT ADC 0 (REG 0x20): 0x%x\n", charger_adc.vcellbot_adc_0.raw);

        v_print |= (charger_adc.vbus_adc_1.raw << 8);
        v_print |= (charger_adc.vbus_adc_0.raw);
        printf("VBUS = %d mV\n", v_print);

        v_print=0;

        v_print |= (charger_adc.vbat_adc_1.raw << 8);
		v_print |= (charger_adc.vbat_adc_0.raw);
		printf("VBAT = %d mV\n", v_print);

		v_print=0;

		v_print |= (charger_adc.vcelltop_adc_1.raw << 8);
		v_print |= (charger_adc.vcelltop_adc_0.raw);
		printf("VCELLTOP = %d mV\n", v_print);

		v_print=0;

		v_print |= (charger_adc.vcellbot_adc_1.raw << 8);
		v_print |= (charger_adc.vcellbot_adc_0.raw);
		printf("VCELLBOT = %d mV\n", v_print);

      }

  return status;
}

status_t
battery_charger_set_adc_enable(
  battery_charger_handle_t* handle,
  bool enable
  )
{
	adc_control_t adc_control = { .raw = 0 };
	status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
	REG_ADC_CONTROL, &adc_control.raw);

	if (status == kStatus_Success) {
		adc_control.adc_en = enable;
		status = i2c_mem_write_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_ADC_CONTROL, adc_control.raw);
	}

	return status;
}

// Local function definitions

static status_t
battery_charger_reset_device(battery_charger_handle_t* handle)
{
	part_information_t part_information = { .reg_rst = true };
	status_t status = i2c_mem_write_byte(handle->i2c_handle, BQ25887_ADDR,
	  REG_PART_INFORMATION, part_information.raw);
	return status;
}

static status_t
battery_charger_reset_watchdog(battery_charger_handle_t* handle)
{
  charger_control_3_t charger_control_3;
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
    REG_CHARGER_CONTROL_3, &charger_control_3.raw);
  if (status == kStatus_Success) {
	  charger_control_3.wd_rst = true;
    status = i2c_mem_write_byte(handle->i2c_handle, BQ25887_ADDR,
    		REG_CHARGER_CONTROL_3, charger_control_3.raw);
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
  // Read status registers.

  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
    REG_CHARGER_STATUS_1, &p_charger_status->charger_status_1.raw);

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
      REG_CHARGER_STATUS_2, &p_charger_status->charger_status_2.raw);
  }

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
      REG_NTC_STATUS, &p_charger_status->ntc_status.raw);
  }

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
      REG_FAULT_STATUS, &p_charger_status->fault_status.raw);
  }

  return status;
}

static status_t
battery_charger_read_control_registers(
  battery_charger_handle_t* handle,
  charger_control_t* p_charger_control
  )
{
  // Read control registers.
  status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
    REG_CHARGER_CONTROL_1, &p_charger_control->charger_control_1.raw);

  if (status == kStatus_Success) {
    status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
      REG_CHARGER_CONTROL_3, &p_charger_control->charger_control_3.raw);
  }

  return status;
}

static status_t
battery_charger_read_adc_registers(
  battery_charger_handle_t* handle,
  charger_adc_t* p_charger_adc
  )
{
	// Read adc registers.
	status_t status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
	REG_ADC_CONTROL, &p_charger_adc->adc_control.raw);

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VBUS_ADC_1, &p_charger_adc->vbus_adc_1.raw);
	}

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VBUS_ADC_0, &p_charger_adc->vbus_adc_0.raw);
	}

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VBAT_ADC_1, &p_charger_adc->vbat_adc_1.raw);
	}

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VBAT_ADC_0, &p_charger_adc->vbat_adc_0.raw);
	}

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VCELLTOP_ADC_1, &p_charger_adc->vcelltop_adc_1.raw);
	}

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VCELLTOP_ADC_0, &p_charger_adc->vcelltop_adc_0.raw);
	}

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VCELLBOT_ADC_1, &p_charger_adc->vcellbot_adc_1.raw);
	}

	if (status == kStatus_Success) {
		status = i2c_mem_read_byte(handle->i2c_handle, BQ25887_ADDR,
				REG_VCELLBOT_ADC_0, &p_charger_adc->vcellbot_adc_0.raw);
	}
	return status;
}

