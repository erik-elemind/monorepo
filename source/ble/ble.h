#ifndef BLE_H
#define BLE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Size of electrode quality characteristic, in bytes
#define ELECTRODE_NUM 8
/// Size of the blink status characteristic, in bytes
#define BLINK_NUM 10
/// Size of the alarm characteristic, in bytes
#define ALARM_NUM 3
/// Size of the time characteristic, in bytes
#define TIME_NUM 8
/// Size of the ble address, in bytes
#define ADDR_NUM 6
/// Number of characters in the serial number characteristic, in bytes
#define SERIAL_NUMBER_NUM 64

typedef union {
  struct {
    // lsb
    uint8_t sat : 1;
    uint8_t fri : 1;
    uint8_t thu : 1;
    uint8_t wed : 1;
    uint8_t tue : 1;
    uint8_t mon : 1;
    uint8_t sun : 1;
    uint8_t on  : 1;
    // msb
  };
  uint8_t byte;
} alarm_flags_t;

typedef union
{
  struct {
    alarm_flags_t flags;
    uint16_t minutes_after_midnight;
  };
  uint8_t all[3];
} alarm_params_t;

// Init called before vTaskStartScheduler() launches our Task in main():
void ble_pretask_init(void);

void ble_task(void *ignored);

// Send various event types to this task:
void ble_battery_level_request(void);
void ble_battery_level_update(uint8_t battery_level);
void ble_serial_number_request(void);
void ble_software_version_request(void);
void ble_dfu_request(void);

void ble_electrode_quality_request(void);
void ble_electrode_quality_update(uint8_t qualities[ELECTRODE_NUM]);
void ble_volume_request(void);
void ble_volume_command(uint8_t volume);
void ble_volume_update(uint8_t volume);
void ble_power_request(void);
void ble_power_command(uint8_t power);
void ble_power_update(uint8_t power);
void ble_therapy_request(void);
void ble_therapy_command(uint8_t therapy);
void ble_therapy_update(uint8_t therapy);
void ble_heart_rate_request(void);
void ble_heart_rate_update(uint8_t heart_rate);
void ble_reset(void);
void ble_blink_status_request(void);
void ble_blink_status_update(uint8_t status[BLINK_NUM]);
void ble_quality_check_request(void);
void ble_quality_check_command(uint8_t quality_check);
void ble_quality_check_update(uint8_t quality_check);
void ble_alarm_request(void);
void ble_alarm_command(alarm_params_t *params);
void ble_alarm_update(alarm_params_t *params);
void ble_sound_request(void);
void ble_sound_command(uint8_t sound);
void ble_sound_update(uint8_t sound);
void ble_time_request(void);
void ble_time_command(uint64_t unix_epoch_time_sec);
void ble_time_update(uint64_t unix_epoch_time_sec);
void ble_charger_status_request(void);
void ble_charger_status_update(uint8_t charger_status);
void ble_settings_request(void);
void ble_settings_command(uint8_t settings);
void ble_settings_update(uint8_t settings);
void ble_memory_level_request(void);
void ble_memory_level_update(uint8_t memory_level);
void ble_factory_reset_request(void);
void ble_factory_reset_command(uint8_t factory_reset);
void ble_factory_reset_update(uint8_t factory_reset);
void ble_sound_control_request(void);
void ble_sound_control_command(uint8_t sound_control);
void ble_sound_control_update(uint8_t sound_control);
void ble_addr_command(uint8_t* addr);
void ble_connected_event(void);
void ble_disconnected_event(void);

uint8_t* ble_get_addr();
bool ble_is_ota_running(void);

// Convenience methods for power off/on
void ble_power_off(void);
void ble_power_on(void);

#ifdef __cplusplus
}
#endif

#endif  // BLE_H
