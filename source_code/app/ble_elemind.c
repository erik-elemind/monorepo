/*
 * ble_elemind.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Elemind Data Service implementation.
 *
 */

// Standard library
#include <string.h>

// nRF5 SDK
#include "app_timer.h"
#include "ble_conn_params.h"
#include "ble_srv_common.h"
#include "boards.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_pwr_mgmt.h"
#include "sdk_common.h"

// Morpheus
#include "ble_elemind.h"
#include "lpc_uart.h"
#include "lpc_reset_timing.h"


/** LPC ISPn pin disable timer. */
APP_TIMER_DEF(m_lpc_ispn_disable_timer_id);


/** LPC reset characteristic value to request ISP mode. */
#define LPC_RESET_ISP_REQUESTED 0x01

/** BLE Update characteristic value to request DFU mode. */
#define BLE_UPDATE_REQUESTED 0x01

/** DFU Reset Delay timer. */
APP_TIMER_DEF(m_dfu_reset_delay_timer_id);

/** DFU Reset Delay time (in milliseconds). */
#define DFU_RESET_DELAY_MS APP_TIMER_TICKS(100)

/** Max serial number length (in characters). */
#define MAX_SERIAL_NUMBER_LEN 64

/** Max software version length (in characters). */
#define MAX_SOFTWARE_VERSION_LEN 64

/** For dev kit builds, use arbitrary free pins for the LPC pin control interface.
 *  See diagram here for the PCA10040:
 *  https://infocenter.nordicsemi.com/topic/ug_nrf52832_dk/UG/nrf52_DK/hw_conn_interf.html
 *  For FF1 and beyond, use the values in custom_board.h.
 */
#ifdef BOARD_PCA10040
#define LPC_RESETN_PIN 11
#define LPC_ISPN_PIN 12
#endif

/** Characteristic access types. */
typedef enum {
  ELEMIND_ACCESS_READ = 1 << 0,
  ELEMIND_ACCESS_WRITE = 1 << 1,
  ELEMIND_ACCESS_NOTIFY = 1 << 2
} elemind_access_t;


/** Serial number. */
static char g_serial_number[MAX_SERIAL_NUMBER_LEN] = "<unknown>";

/** Software version. */
static char g_software_version[MAX_SOFTWARE_VERSION_LEN] = "<unknown>";


/** Handle the LPC ISPn disable timer timeout.

    This function will be called when the LPC ISPn disable
    timer expires. We hold the LPC ISPn line low after performing an
    ISP reset, so that the LPC boot code has time to read the line and
    switch to ISP mode.

    @param p_context Pointer used for passing some arbitrary
    information (context) from the app_start_timer() call to the
    timeout handler.
*/
static void
lpc_ispn_disable_timeout_handler(void * p_context)
{
  UNUSED_PARAMETER(p_context);

  // Disable LPC ISPn line
  nrf_gpio_pin_set(LPC_ISPN_PIN);
}

/** Handle the DFU Reset Delay timer timeout.

    This function will be called when the LPC ISPn disable
    timer expires. We hold the LPC ISPn line low after performing an
    ISP reset, so that the LPC boot code has time to read the line and
    switch to ISP mode.

    @param p_context Pointer used for passing some arbitrary
    information (context) from the app_start_timer() call to the
    timeout handler.
*/
static void
dfu_reset_delay_timeout_handler(void * p_context)
{
  UNUSED_PARAMETER(p_context);

  ret_code_t err_code;

  // Set GPREGRET register to tell bootloader to start DFU
  err_code = sd_power_gpregret_clr(0, 0xffffffff);
  APP_ERROR_CHECK(err_code);

  err_code = sd_power_gpregret_set(0, BOOTLOADER_DFU_START);
  APP_ERROR_CHECK(err_code);

  // Signal that DFU mode is to be enter to the power management module
  nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);
}

/** Add fixed-length characteristic with Elemind base UUID.

    @param p_elemind Elemind service info
    @param p_elemind_init Elemind initialization info
    @param p_char_handles Handles for characteristic to add
    @param uuid UUID for characteristic
    @param p_initial_value Initial value for characteristic
    @param p_value_length Value length (for initial value and max value len)
    @param access Access types (can be ORed together)

    @return NRF_SUCCESS if successful, error code otherwise.
*/
static ret_code_t
char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init,
  ble_gatts_char_handles_t* p_char_handles,
  uint16_t uuid,
  uint8_t* p_initial_value,
  uint16_t value_length,
  elemind_access_t access
  )
{
  ret_code_t err_code;
  ble_add_char_params_t add_char_params;

  VERIFY_PARAM_NOT_NULL(p_elemind);
  VERIFY_PARAM_NOT_NULL(p_elemind_init);

  memset(&add_char_params, 0, sizeof(add_char_params));
  add_char_params.uuid = uuid;
  add_char_params.uuid_type = p_elemind->uuid_type;
  add_char_params.max_len = value_length;
  add_char_params.init_len = value_length;
  add_char_params.p_init_value = p_initial_value;
  add_char_params.is_var_len = false;
  add_char_params.char_props.read = (access & ELEMIND_ACCESS_READ) ? 1 : 0;
  add_char_params.char_props.write = (access & ELEMIND_ACCESS_WRITE) ? 1 : 0;
  add_char_params.char_props.notify = (access & ELEMIND_ACCESS_NOTIFY) ? 1 : 0;

  add_char_params.read_access = p_elemind_init->security_level;
  add_char_params.write_access = p_elemind_init->security_level;
  add_char_params.cccd_write_access = p_elemind_init->security_level;

  err_code = characteristic_add(p_elemind->service_handle, &add_char_params,
    p_char_handles);

  return err_code;
}

/** Add single-byte characteristic with Elemind base UUID.

    @param p_elemind Elemind service info
    @param p_elemind_init Elemind initialization info
    @param p_char_handles Handles for characteristic to add
    @param uuid UUID for characteristic
    @param initial_value Initial value for characteristic
    @param access Access types (can be ORed together)

    @return NRF_SUCCESS if successful, error code otherwise.
*/
static ret_code_t
char_add_uint8(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init,
  ble_gatts_char_handles_t* p_char_handles,
  uint16_t uuid,
  uint8_t initial_value,
  elemind_access_t access
  )
{
  return char_add(p_elemind, p_elemind_init, p_char_handles,
    uuid, &initial_value, sizeof(initial_value), access);
}

/** Update fixed-length characteristic, and notify if connected.

    @param p_elemind Elemind service info
    @param value_handle Handle of characteristic value
    @param p_value New characteristic value
    @param value_length Length of new value

    @return NRF_SUCCESS if successful, error code otherwise.
*/
static ret_code_t
char_update(
  ble_elemind_t* p_elemind,
  uint16_t value_handle,
  uint8_t* p_value,
  uint16_t value_length
  )
{
  VERIFY_PARAM_NOT_NULL(p_elemind);

  ret_code_t err_code = NRF_SUCCESS;

  // Initialize value struct.
  ble_gatts_value_t gatts_value;
  memset(&gatts_value, 0, sizeof(gatts_value));
  gatts_value.len     = value_length;
  gatts_value.offset  = 0;
  gatts_value.p_value = p_value;

  // Update database.
  err_code = sd_ble_gatts_value_set(
    p_elemind->conn_handle,
    value_handle,
    &gatts_value);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Send value if connected and notifying.
  if ((p_elemind->conn_handle != BLE_CONN_HANDLE_INVALID)) {
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = gatts_value.offset;
    hvx_params.p_len  = &gatts_value.len;
    hvx_params.p_data = gatts_value.p_value;

    err_code = sd_ble_gatts_hvx(p_elemind->conn_handle, &hvx_params);
  }
  else {
    err_code = NRF_ERROR_INVALID_STATE;
  }

  return err_code;
}

/** Update single-byte characteristic, and notify if connected.

    @param p_elemind Elemind service info
    @param value_handle Handle of characteristic value
    @param value New characteristic value

    @return NRF_SUCCESS if successful, error code otherwise.
*/
static ret_code_t
char_update_uint8(
  ble_elemind_t* p_elemind,
  uint16_t value_handle,
  uint8_t value
  )
{
  return char_update(p_elemind, value_handle, &value, sizeof(value));
}

/** Add Elemind Electrode_Quality characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
electrode_quality_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add(p_elemind, p_elemind_init,
    &p_elemind->electrode_quality_handles,
    ELEMIND_ELECTRODE_QUALITY_CHAR_UUID,
    (uint8_t*)p_elemind_init->electrode_quality_initial_value,
    sizeof(p_elemind_init->electrode_quality_initial_value),
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_NOTIFY
    );
}

/** Add Elemind Volume characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
volume_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->volume_handles,
    ELEMIND_VOLUME_CHAR_UUID,
    p_elemind_init->volume_initial_value,
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_WRITE | ELEMIND_ACCESS_NOTIFY
    );
}

/** Add Elemind Power characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
power_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->power_handles,
    ELEMIND_POWER_CHAR_UUID,
    p_elemind_init->power_initial_value,
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_WRITE | ELEMIND_ACCESS_NOTIFY
    );
}

/** Add Elemind Therapy characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
therapy_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->therapy_handles,
    ELEMIND_THERAPY_CHAR_UUID,
    p_elemind_init->therapy_initial_value,
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_WRITE | ELEMIND_ACCESS_NOTIFY
    );
}

/** Add Elemind Heart Rate characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
hr_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->hr_handles,
    ELEMIND_HEART_RATE_CHAR_UUID,
    p_elemind_init->hr_initial_value,
    ELEMIND_ACCESS_NOTIFY
    );
}

/** Add Elemind BLE Update characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
ble_update_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->ble_update_handles,
    ELEMIND_BLE_UPDATE_CHAR_UUID,
    p_elemind_init->ble_update_initial_value,
    ELEMIND_ACCESS_WRITE
    );
}

/** Add Elemind LPC Reset characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
lpc_reset_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->lpc_reset_handles,
    ELEMIND_LPC_RESET_CHAR_UUID,
    p_elemind_init->lpc_reset_initial_value,
    ELEMIND_ACCESS_WRITE
    );
}

/** Add Elemind Blink_Status characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
blink_status_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add(p_elemind, p_elemind_init,
    &p_elemind->blink_status_handles,
    ELEMIND_BLINK_STATUS_CHAR_UUID,
    (uint8_t*)p_elemind_init->blink_status_initial_value,
    sizeof(p_elemind_init->blink_status_initial_value),
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_NOTIFY
    );
}

/** Add Elemind Quality Check characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
quality_check_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->quality_check_handles,
    ELEMIND_QUALITY_CHECK_CHAR_UUID,
    p_elemind_init->quality_check_initial_value,
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_WRITE | ELEMIND_ACCESS_NOTIFY
    );
}

/** Add Elemind Alarm characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
alarm_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add(p_elemind, p_elemind_init,
    &p_elemind->alarm_handles,
    ELEMIND_ALARM_CHAR_UUID,
    (uint8_t*)p_elemind_init->alarm_initial_value,
    sizeof(p_elemind_init->alarm_initial_value),
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_WRITE
    );
}

/** Add Elemind Sound characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
sound_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add_uint8(
    p_elemind,
    p_elemind_init,
    &p_elemind->sound_handles,
    ELEMIND_SOUND_CHAR_UUID,
    p_elemind_init->sound_initial_value,
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_WRITE
    );
}

/** Add Elemind Time characteristic.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_elemind_init Information needed to initialize the service.
    @return NRF_SUCCESS on success, otherwise an error code.
*/
static ret_code_t
time_char_add(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init
  )
{
  return char_add(p_elemind, p_elemind_init,
    &p_elemind->time_handles,
    ELEMIND_TIME_CHAR_UUID,
    (uint8_t*)p_elemind_init->time_initial_value,
    sizeof(p_elemind_init->time_initial_value),
    ELEMIND_ACCESS_READ | ELEMIND_ACCESS_WRITE
    );
}


/* Initialize the Elemind Service Init to Defaults. */
void
ble_elemind_init_defaults( ble_elemind_init_t* p_elemind_init )
{
  static const uint8_t electrode_quality_defaults[ELECTRODE_QUALITY_SIZE] =
    {0,0,0,0,0,0,0,0};
  memcpy(p_elemind_init->electrode_quality_initial_value,
    electrode_quality_defaults, sizeof(electrode_quality_defaults));
  p_elemind_init->volume_initial_value = 0;
  p_elemind_init->power_initial_value = 0;
  p_elemind_init->therapy_initial_value = 0;
  p_elemind_init->hr_initial_value = HEART_RATE_INVALID;
  p_elemind_init->ble_update_initial_value = 0;
  p_elemind_init->lpc_reset_initial_value = 0;
  static const uint8_t blink_status_defaults[BLINK_STATUS_SIZE] =
    {0,0,0,0,0,0,0,0,0,0};
  memcpy(p_elemind_init->blink_status_initial_value,
    blink_status_defaults, sizeof(blink_status_defaults));
  p_elemind_init->quality_check_initial_value = 0;
}

/* Initialize the Elemind Service. */
ret_code_t
ble_elemind_init(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init,
  ble_advertising_t* p_advertising
  )
{
  ret_code_t err_code;

  if (p_elemind == NULL || p_elemind_init == NULL || p_advertising == NULL) {
    return NRF_ERROR_NULL;
  }

  // Initialize service structure
  p_elemind->conn_handle = BLE_CONN_HANDLE_INVALID;
  p_elemind->p_advertising = p_advertising;

  // Add Elemind Service UUID
  ble_uuid_t ble_uuid;
  ble_uuid128_t base_uuid = {ELEMIND_SERVICE_UUID_BASE};
  err_code = sd_ble_uuid_vs_add(&base_uuid, &p_elemind->uuid_type);
  VERIFY_SUCCESS(err_code);

  ble_uuid.type = p_elemind->uuid_type;
  ble_uuid.uuid = ELEMIND_DATA_SERVICE_UUID;

  // Add the Elemind Service
  err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
    &ble_uuid, &p_elemind->service_handle);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the Electrode_Quality characteristic
  err_code = electrode_quality_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the Volume characteristic
  err_code = volume_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the Power characteristic
  err_code = power_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the Therapy characteristic
  err_code = therapy_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the Heart Rate characteristic
  err_code = hr_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the BLE Update characteristic
  err_code = ble_update_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the LPC Reset characteristic
  err_code = lpc_reset_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }
  
  // Add the Blink_Status characteristic
  err_code = blink_status_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }
  
  // Add the Quality_Check characteristic
  err_code = quality_check_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the Alarm characteristic
  err_code = alarm_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }
  
  // Add the Sound characteristic
  err_code = sound_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Add the Time characteristic
  err_code = time_char_add(p_elemind, p_elemind_init);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Set up LPC reset GPIO (active-low)
  // Use a pullup here since there are other drivers of this net
  nrf_gpio_cfg_input(LPC_RESETN_PIN, NRF_GPIO_PIN_PULLUP);
  // Setup ISP GPIO (active-low)
  nrf_gpio_pin_set(LPC_ISPN_PIN);
  nrf_gpio_cfg_output(LPC_ISPN_PIN);

  // Create LPC ISP disable timer (used to disable ISP after an ISP reset)
  err_code = app_timer_create(&m_lpc_ispn_disable_timer_id,
    APP_TIMER_MODE_SINGLE_SHOT, lpc_ispn_disable_timeout_handler);
  APP_ERROR_CHECK(err_code);

  // Create DFU Reset Delay timer (used when DFU is requested)
  err_code = app_timer_create(&m_dfu_reset_delay_timer_id,
    APP_TIMER_MODE_SINGLE_SHOT, dfu_reset_delay_timeout_handler);
  APP_ERROR_CHECK(err_code);

  return err_code;
}

/** Handle the Connect event.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_ble_evt Event received from the BLE stack.
*/
static void
on_connect(ble_elemind_t* p_elemind, const ble_evt_t* p_ble_evt)
{
  p_elemind->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

/** Handle the Disconnect event.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_ble_evt Event received from the BLE stack.
*/
static void
on_disconnect(ble_elemind_t* p_elemind, const ble_evt_t* p_ble_evt)
{
  UNUSED_PARAMETER(p_ble_evt);
  p_elemind->conn_handle = BLE_CONN_HANDLE_INVALID;
}


/** Send command with one uint8 parameter to LPC.

    @param[in] p_command Command to send
    @param[in] value Command parameter

    @return NRF_SUCCESS if transmission successful, otherwise an error
    code.
*/
ret_code_t
on_write_command_helper(const char* p_command, uint8_t value)
{
  char line[LPC_MAX_COMMAND_SIZE] = { 0 };
  snprintf(line, sizeof(line)-1, "%s %d\n", p_command, value);
  return lpc_uart_sendline(line);
}

/** Handle Volume characteristic write.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_volume(const ble_gatts_evt_write_t* p_evt_write)
{
  ret_code_t err_code = NRF_SUCCESS;
  err_code = on_write_command_helper("ble_volume", p_evt_write->data[0]);
  APP_ERROR_CHECK(err_code);
}

/** Handle Power characteristic write.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_power(const ble_gatts_evt_write_t* p_evt_write)
{
  ret_code_t err_code = NRF_SUCCESS;
  err_code = on_write_command_helper("ble_power", p_evt_write->data[0]);
  APP_ERROR_CHECK(err_code);
}

/** Handle Therapy characteristic write.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_therapy(const ble_gatts_evt_write_t* p_evt_write)
{
  ret_code_t err_code = NRF_SUCCESS;
  err_code = on_write_command_helper("ble_therapy", p_evt_write->data[0]);
  APP_ERROR_CHECK(err_code);
}

/** Handle BLE Update characteristic write.

    @param[in] p_elemind Elemind Data Service object
    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_ble_update(
  ble_elemind_t* p_elemind,
  const ble_gatts_evt_write_t* p_evt_write
  )
{
  UNUSED_PARAMETER(p_evt_write);

  ret_code_t err_code = ble_elemind_dfu(p_elemind);
  APP_ERROR_CHECK(err_code);
}

/** Handle LPC Reset characteristic write.

    If we're just resetting the LPC, just hold RESETn low for a few
    microseconds (see LPC_RESETN_HOLD_US). Delaying for a few
    microseconds here is ok--we don't have any realtime constraints
    other responding to BLE events.

    If we're putting the LPC into ISP mode, assert ISPn before
    asserting RESETn, and hold it for a few milliseconds (see
    LPC_ISPN_HOLD_MS) after we release RESETn so that it's still low
    when the LPC boot ROM checks it. Since delaying here for multiple
    milliseconds could cause us to miss BLE events and disconnect from
    the BLE central, we start a timer and release ISPn in the timer
    handler.

    This routine mirrors lpc_reset_isp() in lpc_update.c.
    Any updates here should be considered for that function as well.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_lpc_reset(const ble_gatts_evt_write_t* p_evt_write)
{
  if (p_evt_write->data[0] == LPC_RESET_ISP_REQUESTED) {
    nrf_gpio_pin_clear(LPC_ISPN_PIN);
  }
  nrf_gpio_pin_clear(LPC_RESETN_PIN);
  nrf_gpio_cfg_output(LPC_RESETN_PIN);
  nrf_delay_us(LPC_RESETN_HOLD_US); // Hold reset low for a little bit
  nrf_gpio_cfg_input(LPC_RESETN_PIN, NRF_GPIO_PIN_PULLUP);

  if (p_evt_write->data[0] == LPC_RESET_ISP_REQUESTED) {
    // Hold ISP low for longer, so that boot code has time to read it
    ret_code_t err_code = app_timer_start(m_lpc_ispn_disable_timer_id,
      APP_TIMER_TICKS(LPC_ISPN_HOLD_TIME_MS), NULL);
    APP_ERROR_CHECK(err_code);
  }
}

/** Handle Quality_Check characteristic write.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_quality_check(const ble_gatts_evt_write_t* p_evt_write)
{
  ret_code_t err_code = NRF_SUCCESS;
  err_code = on_write_command_helper("ble_quality_check", p_evt_write->data[0]);
  APP_ERROR_CHECK(err_code);
}

/** Handle Alarm characteristic write.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_alarm(const ble_gatts_evt_write_t* p_evt_write)
{
  ret_code_t err_code = NRF_SUCCESS;

  if (p_evt_write->len < 3){
    err_code = NRF_ERROR_DATA_SIZE;

  }else{
    uint8_t  byte0 = p_evt_write->data[0];
    uint16_t byte1 = p_evt_write->data[1];
    uint16_t byte2 = p_evt_write->data[2];
    bool on  = (byte0>>7) & 0x01;
    bool sun = (byte0>>6) & 0x01;
    bool mon = (byte0>>5) & 0x01;
    bool tue = (byte0>>4) & 0x01;
    bool wed = (byte0>>3) & 0x01;
    bool thu = (byte0>>2) & 0x01;
    bool fri = (byte0>>1) & 0x01;
    bool sat = (byte0>>0) & 0x01;
    uint16_t minutes_after_midnight = ((byte2 & 0x00FF)<<8) | ((byte1 & 0x00FF)<<0);
    
    char line[LPC_MAX_COMMAND_SIZE] = { 0 };
    int num_written = snprintf(line, sizeof(line)-1, "%s %d %d %d %d %d %d %d %d %d\n", "ble_alarm",
       on, sun, mon, tue, wed, thu, fri, sat,
       minutes_after_midnight);

    if(num_written<=0){
      err_code = NRF_ERROR_INVALID_STATE;
    }else{
      err_code = lpc_uart_send_buffer( (uint8_t*)line, num_written );
    }
  }

  APP_ERROR_CHECK(err_code);
}

/** Handle Sound characteristic write.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_sound(const ble_gatts_evt_write_t* p_evt_write)
{
  ret_code_t err_code = NRF_SUCCESS;
  err_code = on_write_command_helper("ble_sound", p_evt_write->data[0]);
  APP_ERROR_CHECK(err_code);
}

/** Handle Time characteristic write.

    @param[in] p_evt_write Write event received from the BLE stack.
*/
static void
on_write_time(const ble_gatts_evt_write_t* p_evt_write)
{
  ret_code_t err_code = NRF_SUCCESS;

  if (p_evt_write->len < 8){
    err_code = NRF_ERROR_DATA_SIZE;

  }else{
    uint64_t unix_epoch_time_sec = 
      ((((uint64_t)(p_evt_write->data[7])) & 0x00000000000000FF)<<56) | 
      ((((uint64_t)(p_evt_write->data[6])) & 0x00000000000000FF)<<48) | 
      ((((uint64_t)(p_evt_write->data[5])) & 0x00000000000000FF)<<40) | 
      ((((uint64_t)(p_evt_write->data[4])) & 0x00000000000000FF)<<32) | 
      ((((uint64_t)(p_evt_write->data[3])) & 0x00000000000000FF)<<24) | 
      ((((uint64_t)(p_evt_write->data[2])) & 0x00000000000000FF)<<16) | 
      ((((uint64_t)(p_evt_write->data[1])) & 0x00000000000000FF)<<8)  | 
      ((((uint64_t)(p_evt_write->data[0])) & 0x00000000000000FF)<<0);
    
    char line[LPC_MAX_COMMAND_SIZE] = { 0 };
    // TODO: use full newlib library to get the ability to print 64 bit int
    int num_written = snprintf(line, sizeof(line)-1, "%s %lu\n", "ble_time", (uint32_t)unix_epoch_time_sec);

    if(num_written<=0){
      err_code = NRF_ERROR_INVALID_STATE;
    }else{
      err_code = lpc_uart_send_buffer( (uint8_t*)line, num_written );
    }
  }

  APP_ERROR_CHECK(err_code);
}


/** Handle the Write event.

    @param[in] p_elemind Elemind Service structure.
    @param[in] p_ble_evt Event received from the BLE stack.
*/
static void on_write(ble_elemind_t* p_elemind, const ble_evt_t* p_ble_evt)
{
  const ble_gatts_evt_write_t* p_evt_write =
    &p_ble_evt->evt.gatts_evt.params.write;

  /* Check if the handle passed with the event matches one of the Elemind
     service writable characteristic handles. */
  if (p_evt_write->handle == p_elemind->volume_handles.value_handle) {
    NRF_LOG_INFO("Volume written: %d", p_evt_write->data[0]);
    on_write_volume(p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->power_handles.value_handle) {
    NRF_LOG_INFO("Power written: %d", p_evt_write->data[0]);
    on_write_power(p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->therapy_handles.value_handle) {
    NRF_LOG_INFO("Therapy written: %d", p_evt_write->data[0]);
    on_write_therapy(p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->ble_update_handles.value_handle) {
    NRF_LOG_INFO("BLE Update written: %d", p_evt_write->data[0]);
    on_write_ble_update(p_elemind, p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->lpc_reset_handles.value_handle) {
    NRF_LOG_INFO("LPC Reset written: %d", p_evt_write->data[0]);
    on_write_lpc_reset(p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->quality_check_handles.value_handle) {
    NRF_LOG_INFO("Quality Check written: %d", p_evt_write->data[0]);
    on_write_quality_check(p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->alarm_handles.value_handle) {
    NRF_LOG_INFO("Alarm written: %d", p_evt_write->data[0]);
    on_write_alarm(p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->sound_handles.value_handle) {
    NRF_LOG_INFO("Sound written: %d", p_evt_write->data[0]);
    on_write_sound(p_evt_write);
  }
  else if (p_evt_write->handle == p_elemind->time_handles.value_handle) {
    NRF_LOG_INFO("Time written: %d", p_evt_write->data[0]);
    on_write_time(p_evt_write);
  }
}

/* Handle Elemind Data Service BLE Stack events. */
void
ble_elemind_on_ble_evt(const ble_evt_t* p_ble_evt, void* p_context)
{
  ble_elemind_t* p_elemind = (ble_elemind_t*) p_context;

  if (p_elemind == NULL || p_ble_evt == NULL) {
    return;
  }

  switch (p_ble_evt->header.evt_id)
    {
      case BLE_GAP_EVT_CONNECTED:
        on_connect(p_elemind, p_ble_evt);
        break;

      case BLE_GAP_EVT_DISCONNECTED:
        on_disconnect(p_elemind, p_ble_evt);
        break;

      case BLE_GATTS_EVT_WRITE:
        on_write(p_elemind, p_ble_evt);
        break;

      default:
        // No implementation needed.
        break;
    }
}

/* Update the Serial Number value. */
ret_code_t
ble_elemind_serial_number_update(
  ble_elemind_t* p_elemind,
  char* serial_number_value
  )
{
  static bool initialized = false;

  VERIFY_PARAM_NOT_NULL(serial_number_value);

  if (!initialized) {
    initialized = true;
    strncpy(g_serial_number, serial_number_value,
      ARRAY_SIZE(g_serial_number)-1);
    g_serial_number[ARRAY_SIZE(g_serial_number)-1] = '\0';
  }
  else {
    NRF_LOG_ERROR("Serial number received more than once!");
  }

  return NRF_SUCCESS;
}

/* Update the Software Version value. */
ret_code_t
ble_elemind_software_version_update(
  ble_elemind_t* p_elemind,
  char* software_version_value
  )
{
  static bool initialized = false;

  VERIFY_PARAM_NOT_NULL(software_version_value);

  if (!initialized) {
    initialized = true;
    strncpy(g_software_version, software_version_value,
      ARRAY_SIZE(g_software_version)-1);
    g_software_version[ARRAY_SIZE(g_software_version)-1] = '\0';
  }
  else {
    NRF_LOG_ERROR("Software version received more than once!");
  }

  return NRF_SUCCESS;
}

/* Update the Electrode_Quality value. */
ret_code_t
ble_elemind_electrode_quality_update(
  ble_elemind_t* p_elemind,
  uint8_t* p_electrode_quality_value
  )
{
  return char_update(p_elemind,
    p_elemind->electrode_quality_handles.value_handle,
    p_electrode_quality_value, ELECTRODE_QUALITY_SIZE);
}

/* Update the Volume value. */
ret_code_t
ble_elemind_volume_update(ble_elemind_t* p_elemind, uint8_t volume_value)
{
  return char_update_uint8(p_elemind,
    p_elemind->volume_handles.value_handle, volume_value);
}

/* Update the Power value. */
ret_code_t
ble_elemind_power_update(ble_elemind_t* p_elemind, uint8_t power_value)
{
  return char_update_uint8(p_elemind,
    p_elemind->power_handles.value_handle, power_value);
}

/* Update the Therapy value. */
ret_code_t
ble_elemind_therapy_update(ble_elemind_t* p_elemind, uint8_t therapy_value)
{
  return char_update_uint8(p_elemind,
    p_elemind->therapy_handles.value_handle, therapy_value);
}

/* Update the Heart Rate value. */
ret_code_t
ble_elemind_hr_update(ble_elemind_t* p_elemind, uint8_t hr_value)
{
  return char_update_uint8(p_elemind,
    p_elemind->hr_handles.value_handle, hr_value);
}

/* Update the Blink_Status value. */
ret_code_t
ble_elemind_blink_status_update(
  ble_elemind_t* p_elemind,
  uint8_t* p_blink_status_value
  )
{
  return char_update(p_elemind,
    p_elemind->blink_status_handles.value_handle,
    p_blink_status_value, BLINK_STATUS_SIZE);
}

/* Update the Quality Check value. */
ret_code_t
ble_elemind_quality_check_update(
  ble_elemind_t* p_elemind, 
  uint8_t quality_check)
{
    return char_update_uint8(p_elemind,
    p_elemind->quality_check_handles.value_handle, quality_check);
}

/* Update the Alarm value. */
ret_code_t
ble_elemind_alarm_update(
  ble_elemind_t* p_elemind,
  uint8_t* p_alarm_value
  )
{
  return char_update(p_elemind,
    p_elemind->alarm_handles.value_handle,
    p_alarm_value, ALARM_SIZE);
}

/* Update the Sound value. */
ret_code_t
ble_elemind_sound_update(
  ble_elemind_t* p_elemind, 
  uint8_t sound)
{
    return char_update_uint8(p_elemind,
    p_elemind->sound_handles.value_handle, sound);
}

/* Update the Time value. */
ret_code_t
ble_elemind_time_update(
  ble_elemind_t* p_elemind,
  uint8_t* p_time_value
  )
{
  return char_update(p_elemind,
    p_elemind->time_handles.value_handle,
    p_time_value, TIME_SIZE);
}


/* Get the Serial Number value. */
char*
ble_elemind_get_serial_number()
{
  return g_serial_number;
}

/* Get the Software Version value. */
char*
ble_elemind_get_software_version()
{
  return g_software_version;
}

/* Put the system into DFU mode. */
ret_code_t
ble_elemind_dfu(ble_elemind_t* p_elemind)
{
  ret_code_t err_code = NRF_SUCCESS;

  NRF_LOG_INFO("Starting DFU");

  // Disconnect from BLE peer if connected
  // If not connect, stop advertising if enabled
  if (p_elemind->conn_handle != BLE_CONN_HANDLE_INVALID) {
    // Disconnect from peer.
    err_code = sd_ble_gap_disconnect(p_elemind->conn_handle,
      BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    VERIFY_SUCCESS(err_code);
  }
  else {
    // If not connected, stop advertising.
    err_code = sd_ble_gap_adv_stop(p_elemind->p_advertising->adv_handle);
    VERIFY_SUCCESS(err_code);
  }
  err_code = ble_conn_params_stop();
  VERIFY_SUCCESS(err_code);

  // Schedule reset after BLE connection has had time to update
  err_code = app_timer_start(m_dfu_reset_delay_timer_id,
    DFU_RESET_DELAY_MS, NULL);
  VERIFY_SUCCESS(err_code);

  return err_code;
}
