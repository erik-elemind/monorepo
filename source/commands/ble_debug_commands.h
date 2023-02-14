/*
 * ble_debug_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for BLE interface.
 *
 */
#ifndef BLE_DEBUG_COMMANDS_H
#define BLE_DEBUG_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void ble_ping_command(int argc, char **argv);

void ble_reset_debug_command(int argc, char **argv);
void ble_dfu_debug_command(int argc, char **argv);

void ble_electrode_quality_debug_command(int argc, char **argv);
void ble_blink_status_debug_command(int argc, char **argv);
void ble_volume_debug_command(int argc, char **argv);
void ble_power_debug_command(int argc, char **argv);
void ble_therapy_debug_command(int argc, char **argv);
void ble_heart_rate_debug_command(int argc, char **argv);
void ble_battery_level_debug_command(int argc, char **argv);
void ble_quality_check_debug_command(int argc, char **argv);
void ble_alarm_debug_command(int argc, char **argv);
void ble_sound_debug_command(int argc, char **argv);
void ble_time_debug_command(int argc, char **argv);

void ble_send_file_command(int argc, char **argv);

void ble_charger_status_debug_command(int argc, char **argv);
void ble_settings_debug_command(int argc, char **argv);
void ble_memory_level_debug_command(int argc, char **argv);
void ble_factory_reset_debug_command(int argc, char **argv);
void ble_sound_control_debug_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif  // BLE_DEBUG_COMMANDS
