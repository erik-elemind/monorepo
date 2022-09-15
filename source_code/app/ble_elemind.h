/*
 * ble_elemind.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Elemind Data Service implementation.
 *
 */
#ifndef __BLE_ELEMIND_H__
#define __BLE_ELEMIND_H__

#include <stdint.h>
#include <stdbool.h>

#include "ble.h"
#include "ble_advertising.h"
#include "ble_srv_common.h"


/** Elemind base UUID: D640xxxx-398C-4EB9-8889-15D0B9A1CCDD
    Given in reversed (little-endian) order, per nRF5 SDK. */
#define ELEMIND_SERVICE_UUID_BASE                       \
  {0xDD, 0xCC, 0xA1, 0xB9, 0xD0, 0x15, 0x89, 0x88,      \
   0xB9, 0x4E, 0x8C, 0x39, 0x00, 0x00, 0x40, 0xD6}

/// Elemind data service 16-bit ID (used with base UUID, above)
#define ELEMIND_DATA_SERVICE_UUID 0x0000

/// Elemind data service characteristic 16-bit IDs (used with base UUID, above)
#define ELEMIND_ELECTRODE_QUALITY_CHAR_UUID 0x0001
#define ELEMIND_VOLUME_CHAR_UUID 0x0002
#define ELEMIND_POWER_CHAR_UUID 0x0003
#define ELEMIND_THERAPY_CHAR_UUID 0x0004
#define ELEMIND_HEART_RATE_CHAR_UUID 0x0005
#define ELEMIND_BLE_UPDATE_CHAR_UUID 0x0006
#define ELEMIND_LPC_RESET_CHAR_UUID 0x0007
#define ELEMIND_BLINK_STATUS_CHAR_UUID 0x0008
#define ELEMIND_QUALITY_CHECK_CHAR_UUID 0x0009
#define ELEMIND_ALARM_CHAR_UUID 0x000A
#define ELEMIND_SOUND_CHAR_UUID 0x000B
#define ELEMIND_TIME_CHAR_UUID 0x000C

/** BLE event observer priority.

    Controls when ble_elemind_on_ble_evt() is called, relative to
    other event handlers. */
#define ELEMIND_BLE_OBSERVER_PRIO 2

/// Size of electrode quality characteristic, in bytes
#define ELECTRODE_QUALITY_SIZE 8

/// Size of the blink status characteristic, in bytes
#define BLINK_STATUS_SIZE 10

/// Size of the alarm characteristic, in bytes
#define ALARM_SIZE 3

/// Size of the time characteristic, in bytes
#define TIME_SIZE 8

/// Invalid heart rate characteristic value
#define HEART_RATE_INVALID 0xFF

/** Macro for defining a ble_elemind instance.

    @param _name Name of the instance.
*/
#define BLE_ELEMIND_DEF(_name)                  \
  static ble_elemind_t _name;                   \
  NRF_SDH_BLE_OBSERVER(_name ## _obs,           \
    ELEMIND_BLE_OBSERVER_PRIO,                  \
    ble_elemind_on_ble_evt, &_name)


/** Elemind Service init structure.

    This contains all options and data needed for initialization of
    the service.
*/
typedef struct
{
  /** Security level (used for all characteristics). */
  security_req_t security_level;

  /** Initial electrode_quality */
  uint8_t electrode_quality_initial_value[ELECTRODE_QUALITY_SIZE];

  /** Initial volume */
  uint8_t volume_initial_value;

  /** Initial power */
  uint8_t power_initial_value;

  /** Initial therapy */
  uint8_t therapy_initial_value;

  /** Initial heart rate  */
  uint8_t hr_initial_value;

  /** Initial BLE update value  */
  uint8_t ble_update_initial_value;

  /** Initial BLE update value  */
  uint8_t lpc_reset_initial_value;
  
  /** Initial blink_status */
  uint8_t blink_status_initial_value[BLINK_STATUS_SIZE];
  
  /** Initial quality_check */
  uint8_t quality_check_initial_value;

  /** Initial alarm */
  uint8_t alarm_initial_value[ALARM_SIZE];

  /** Initial sound */
  uint8_t sound_initial_value;

  /** Initial time */
  uint8_t time_initial_value[TIME_SIZE];

} ble_elemind_init_t;

/** Elemind Service structure.

    This contains service, characteristic, and connection information
    for the service.
*/
typedef struct
{
  /** Handle of Elemind Service (as provided by the BLE stack). */
  uint16_t service_handle;

  /** Handles related to the Elemind Electrode_Quality characteristic. */
  ble_gatts_char_handles_t electrode_quality_handles;

  /** Handles related to the Elemind Volume characteristic. */
  ble_gatts_char_handles_t volume_handles;

  /** Handles related to the Elemind Power characteristic. */
  ble_gatts_char_handles_t power_handles;

  /** Handles related to the Elemind Therapy characteristic. */
  ble_gatts_char_handles_t therapy_handles;

  /** Handles related to the Elemind Heart Rate characteristic. */
  ble_gatts_char_handles_t hr_handles;

  /** Handles related to the Elemind BLE Update characteristic. */
  ble_gatts_char_handles_t ble_update_handles;

  /** Handles related to the Elemind LPC Reset characteristic. */
  ble_gatts_char_handles_t lpc_reset_handles;
  
  /** Handles related to the Elemind Blink_Status characteristic. */
  ble_gatts_char_handles_t blink_status_handles;
  
  /** Handles related to the Elemind Quality_Check characteristic. */
  ble_gatts_char_handles_t quality_check_handles;

  /** Handles related to the Elemind Alarm characteristic. */
  ble_gatts_char_handles_t alarm_handles;

  /** Handles related to the Elemind Sound characteristic. */
  ble_gatts_char_handles_t sound_handles;

  /** Handles related to the Elemind Time characteristic. */
  ble_gatts_char_handles_t time_handles;

  /** Handle of the current connection (as provided by the BLE stack,
      is BLE_CONN_HANDLE_INVALID if not in a connection). */
  uint16_t conn_handle;

  /** Advertising data (used to stop advertising on DFU). */
  ble_advertising_t* p_advertising;

  /** Custom UUID type (for creating 128-bit UUIDs). */
  uint8_t uuid_type;

} ble_elemind_t;

/** Initialize the Elemind Service Init to Defaults.

    @param[out] p_elemind Elemind Service Init structure. This structure
    will have to be supplied by the application. It will be
    initialized to default values in this function.
*/
void
ble_elemind_init_defaults(
  ble_elemind_init_t* p_elemind_init
  );

/** Initialize the Elemind Service.

    @param[out] p_elemind Elemind Service structure. This structure
    will have to be supplied by the application. It will be
    initialized by this function, and will later be used to identify
    this particular service instance.

    @param[in] p_elemind_init Information needed to initialize the service.

    @return NRF_SUCCESS on successful initialization of service,
    otherwise an error code.
*/
ret_code_t
ble_elemind_init(
  ble_elemind_t* p_elemind,
  const ble_elemind_init_t* p_elemind_init,
  ble_advertising_t* p_advertising
  );

/** Handle Elemind Data Service BLE Stack events.

    Handles all events from the BLE stack of interest to the Elemind
    Data Service.

    @param[in] p_ble_evt Event received from the BLE stack.
    @param[in] p_context Elemind Data Service structure.
*/
void
ble_elemind_on_ble_evt(const ble_evt_t* p_ble_evt, void* p_context);

/** Update the Electrode Quality value.

    The application calls this function when the electrode_quality
    value should be updated. If notification has been enabled, the
    electrode_quality characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] electrode_quality_value New Electrode_Quality value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_electrode_quality_update(
  ble_elemind_t* p_elemind,
  uint8_t* p_electrode_quality_value
  );

/** Update the Volume value.

    The application calls this function when the volume value should
    be updated. If notification has been enabled, the volume
    characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] volume_value New Volume value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_volume_update(ble_elemind_t* p_elemind, uint8_t volume_value);

/** Update the Power value.

    The application calls this function when the power value should be
    updated. If notification has been enabled, the power
    characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] power_value New Power value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_power_update(ble_elemind_t* p_elemind, uint8_t power_value);

/** Update the Therapy value.

    The application calls this function when the therapy value should
    be updated. If notification has been enabled, the therapy
    characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] therapy_value New Therapy value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_therapy_update(ble_elemind_t* p_elemind, uint8_t therapy_value);

/** Update the Heart Rate value.

    The application calls this function when the HR value should be
    updated. If notification has been enabled, the HR characteristic
    is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] hr_value New HR value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_hr_update(ble_elemind_t* p_elemind, uint8_t hr_value);

/** Update the Serial Number value.

    The application calls this function when the serial number value
    should be updated. This should only be called once at startup, so
    that it can be used to initialize the Device Informtion Service;
    subsequent calls are ignored.

    @param[in] p_elemind Elemind Service structure.
    @param[in] serial_number_value New serial number value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_serial_number_update(
  ble_elemind_t* p_elemind,
  char* serial_number_value
  );

/** Update the Software Version value.

    The application calls this function when the software version
    value should be updated. This should only be called once at
    startup, so that it can be used to initialize the Device
    Informtion Service; subsequent calls are ignored..

    @param[in] p_elemind Elemind Service structure.
    @param[in] software_version_value New software version value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_software_version_update(
  ble_elemind_t* p_elemind,
  char* software_version_value
  );

/** Update the Blink Status value.

    The application calls this function when the blink_status
    value should be updated. If notification has been enabled, the
    blink_status characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] blink_status_value New Blink_Status value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_blink_status_update(
  ble_elemind_t* p_elemind,
  uint8_t* p_blink_status_value
  );

/** Update the Quality Check value.

    The application calls this function when the quality_check value should be
    updated. If notification has been enabled, the quality_check
    characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] quality_check New Quality Check value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_quality_check_update(
  ble_elemind_t* p_elemind, 
  uint8_t quality_check
  );

/** Update the Alarm value.

    The application calls this function when the alarm value should be
    updated. If notification has been enabled, the alarm
    characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] alarm New Alarm value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_alarm_update(
  ble_elemind_t* p_elemind, 
  uint8_t* alarm
  );

/** Update the Sound value.

    The application calls this function when the sound value should be
    updated. If notification has been enabled, the sound
    characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] sound New Sound value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_sound_update(
  ble_elemind_t* p_elemind, 
  uint8_t sound
  );

/** Update the Time value.

    The application calls this function when the time value should be
    updated. If notification has been enabled, the time
    characteristic is sent to the client.

    @param[in] p_elemind Elemind Service structure.
    @param[in] time New Time value

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_time_update(
  ble_elemind_t* p_elemind, 
  uint8_t* time
  );

/** Get the Serial Number value.

    This function always returns a valid string; if the serial number
    has been sent from the LPC, it will be a real serial number,
    otherwise it will be a default "unknown" string.

    @return Serial number
*/
char*
ble_elemind_get_serial_number();

/** Get the Software Version value.

    This function always returns a valid string; if the software
    version has been sent from the LPC, it will be a real software
    version, otherwise it will be a default "unknown" string.

    @return Software version
*/
char*
ble_elemind_get_software_version();

/** Put the system into DFU mode.

    Stops advertising and drops connection if connected, then sets
    timer to reset to DFU mode. This function will return; the system
    is not reset until the timer goes off.

    @param[in] p_elemind Elemind Service structure.

    @return NRF_SUCCESS on success, otherwise an error code.
*/
ret_code_t
ble_elemind_dfu(ble_elemind_t* p_elemind);

#endif /* __BLE_ELEMIND_H__ */
