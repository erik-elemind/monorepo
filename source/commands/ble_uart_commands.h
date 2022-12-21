#ifndef BLE_UART_COMMANDS_H
#define BLE_UART_COMMANDS_H

#include <shell_recv.h> // struct shell_command

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_uart_commands.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"

#include "utils.h"
#include "task.h"
#include "command_helpers.h"

#include "ble.h"


void electrode_quality_request(int argc, char **argv);
void volume_request(int argc, char **argv);
void volume_command(int argc, char **argv);
void power_request(int argc, char **argv);
void power_command(int argc, char **argv);
void therapy_request(int argc, char **argv);
void therapy_command(int argc, char **argv);
void heart_rate_request(int argc, char **argv);
void battery_level_request(int argc, char **argv);
void serial_number_request(int argc, char **argv);
void software_version_request(int argc, char **argv);
void quality_check_request(int argc, char **argv);
void quality_check_command(int argc, char **argv);
void alarm_request(int argc, char **argv);
void alarm_command(int argc, char **argv);
void sound_request(int argc, char **argv);
void sound_command(int argc, char **argv);
void time_request(int argc, char **argv);
void time_command(int argc, char **argv);
void addr_command(int argc, char **argv);
void ble_print_addr_command(int argc, char **argv);
void ble_connected(int argc, char **argv);
void ble_disconnected(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif  // BLE_UART_COMMANDS_H
