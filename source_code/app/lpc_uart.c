/*
 * lpc_uart.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: LPC UART interface for Elemind Morpheus.
 *
 * Based on shell from BLE task on LPC.
 *
 */

// Standard library
#include <stdlib.h>

// nRF5 SDK
#include "app_timer.h"
#include "app_uart.h"
#include "nrf_log.h"
#include "nrf_pwr_mgmt.h"
#include "sdk_common.h"

// Morpheus
#include "ble_elemind.h"
#include "lpc_uart.h"
#include "binary_interface_inst.h"

/** Maximum number of arguments allowed (any more won't get parsed). */
#define ARGC_MAX 16

/** LPC command name to handler mapping type. */
struct lpc_command {
  char *name;
  void (*function )(int argc, char **argv);
};

/** Battery level service (used to update battery level characteristic). */
static ble_bas_t* g_bas_svc = NULL;

/** Elemind Data service (used to update characteristics). */
static ble_elemind_t* g_elemind = NULL;


/** Parse single-byte integer argument from LPC command.

    @param command Command name (only used to log error)
    @param arg Argument to parse
    @param[out] p_value Byte to receive value if parse succcessful

    @return true if argument parsed successfully, false otherwise.
*/
static bool
parse_uint8_arg(char *command, char *arg, uint8_t *p_value)
{
  char* endptr;
  long raw_value = strtol(arg, &endptr, 0);
  if ((*endptr != '\0') || (raw_value < 0) || (raw_value > UINT8_MAX)) {
    NRF_LOG_ERROR("Error: Command %s: invalid argument %s",
      command, arg);
    return false;
  }
  *p_value = (uint8_t)raw_value;

  return true;
}

/** Parse four-byte integer argument from LPC command.

    @param command Command name (only used to log error)
    @param arg Argument to parse
    @param[out] p_value Byte to receive value if parse succcessful

    @return true if argument parsed successfully, false otherwise.
*/
static bool
parse_uint32_arg(char *command, char *arg, uint32_t *p_value)
{
  char* endptr;
  unsigned long raw_value = strtoul(arg, &endptr, 0);
  if ((*endptr != '\0') || (raw_value < 0) || (raw_value > UINT32_MAX)) {
    NRF_LOG_ERROR("Error: Command %s: invalid argument %s",
      command, arg);
    return false;
  }
  *p_value = (uint32_t)raw_value;

  return true;
}


/** Parse eight-byte integer argument from LPC command.

    @param command Command name (only used to log error)
    @param arg Argument to parse
    @param[out] p_value Byte to receive value if parse succcessful

    @return true if argument parsed successfully, false otherwise.
*/
static bool
parse_uint64_arg(char *command, char *arg, uint64_t *p_value)
{
  char* endptr;
  unsigned long long raw_value = strtoull(arg, &endptr, 0);
  if ((*endptr != '\0') || (raw_value < 0) || (raw_value > UINT64_MAX)) {
    NRF_LOG_ERROR("Error: Command %s: invalid argument %s",
      command, arg);
    return false;
  }
  *p_value = (uint64_t)raw_value;

  return true;
}

/** Handle battery level command.

    Update battery level characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_battery_level(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get battery level
  uint8_t battery_level;
  if (parse_uint8_arg(argv[0], argv[1], &battery_level)) {
    ble_bas_battery_level_update(g_bas_svc, battery_level,
      g_elemind->conn_handle);
  }
}

/** Handle serial number command.

    Update serial number characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_serial_number(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  ble_elemind_serial_number_update(g_elemind, argv[1]);
}

/** Handle software version command.

    Update software version characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_software_version(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  ble_elemind_software_version_update(g_elemind, argv[1]);
}

/** Handle DFU command.

    Reset into DFU mode.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_ble_dfu(int argc, char **argv)
{
  if (argc != 1) {
    NRF_LOG_ERROR("Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_elemind_dfu(g_elemind);
}

/** Handle electrode quality command.

    Update electrode quality characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_electrode_quality(int argc, char **argv)
{
  if (argc != (ELECTRODE_QUALITY_SIZE + 1)) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  bool success = true;
  uint8_t electrode_quality[ELECTRODE_QUALITY_SIZE];
  for (int i = 0; (i < ELECTRODE_QUALITY_SIZE) && success; i++) {
    success = parse_uint8_arg(argv[0], argv[i+1], &electrode_quality[i]);
  }

  if (success) {
    ble_elemind_electrode_quality_update(g_elemind, electrode_quality);
  }
}

/** Handle volume command.

    Update volume characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_volume(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get volume
  uint8_t volume;
  if (parse_uint8_arg(argv[0], argv[1], &volume)) {
    ble_elemind_volume_update(g_elemind, volume);
  }
}

/** Handle power command.

    Update power characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_power(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get power
  uint8_t power;
  if (parse_uint8_arg(argv[0], argv[1], &power)) {
    ble_elemind_power_update(g_elemind, power);
  }

  if (power == 0) {
    // Shut down (wake on serial traffic from LPC)
    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
  }
}

/** Handle therapy command.

    Update therapy characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_therapy(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get therapy
  uint8_t therapy;
  if (parse_uint8_arg(argv[0], argv[1], &therapy)) {
    ble_elemind_therapy_update(g_elemind, therapy);
  }
}

/** Handle heart rate command.

    Update heart rate characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_heart_rate(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get heart_rate
  uint8_t hr;
  if (parse_uint8_arg(argv[0], argv[1], &hr)) {
    ble_elemind_hr_update(g_elemind, hr);
  }
}

/** Handle blink status command.

    Update blink status characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_blink_status(int argc, char **argv)
{
  if (argc != (BLINK_STATUS_SIZE + 1)) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  bool success = true;
  uint8_t blink_status[BLINK_STATUS_SIZE];
  for (int i = 0; (i < BLINK_STATUS_SIZE) && success; i++) {
    success = parse_uint8_arg(argv[0], argv[i+1], &blink_status[i]);
  }

  if (success) {
    ble_elemind_blink_status_update(g_elemind, blink_status);
  }
}

/** Handle quality check.

    Update quality characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_quality_check(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get quality
  uint8_t check;
  if (parse_uint8_arg(argv[0], argv[1], &check)) {
    ble_elemind_quality_check_update(g_elemind, check);
  }
}

/** Handle alarm command.

    Update alarm characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_alarm(int argc, char **argv)
{
  if (argc != (9 + 1)) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  bool success = true;
  // Parse alarm
  alarm_params_t params = {.all={0}};
  uint8_t temp = 0;

  /// parse 'on'
  success &= parse_uint8_arg(argv[0], argv[1], &temp);
  params.on = temp > 0;
  /// parse 'sun'
  success &= parse_uint8_arg(argv[0], argv[2], &temp);
  params.sun = temp > 0;
  /// parse 'mon'
  success &= parse_uint8_arg(argv[0], argv[3], &temp);
  params.mon = temp > 0;
  /// parse 'tue'
  success &= parse_uint8_arg(argv[0], argv[4], &temp);
  params.tue = temp > 0;
  /// parse 'wed'
  success &= parse_uint8_arg(argv[0], argv[5], &temp);
  params.wed = temp > 0;
  /// parse 'thu'
  success &= parse_uint8_arg(argv[0], argv[6], &temp);
  params.thu = temp > 0;
  /// parse 'fri'
  success &= parse_uint8_arg(argv[0], argv[7], &temp);
  params.fri = temp > 0;
  /// parse 'sat'
  success &= parse_uint8_arg(argv[0], argv[8], &temp);
  params.sat = temp > 0;

  // parse time_min
  uint32_t time_min = 0;
  success &= parse_uint32_arg(argv[0], argv[9], &time_min);
  params.minutes_after_midnight = (uint16_t) time_min;

  if (success) {
    ble_elemind_alarm_update(g_elemind, params.all);
  }
}

/** Handle sound command.

    Update sound characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_sound(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get sound
  uint8_t sound = 0;
  if (parse_uint8_arg(argv[0], argv[1], &sound)) {
    ble_elemind_sound_update(g_elemind, sound);
  }
}

/** Handle time command.

    Update time characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_time(int argc, char **argv)
{
  if (argc != (1 + 1)) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  uint64_t time = 0;
  uint8_t time_bytes[TIME_SIZE];

  if (parse_uint64_arg(argv[0], argv[1], &time)) {
    memcpy(time_bytes, &time, TIME_SIZE);
    ble_elemind_time_update(g_elemind, time_bytes);
  }
}

/** Handle charger status command.

    Update charger status characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_charger_status(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get charger_status
  uint8_t charger_status = 0;
  if (parse_uint8_arg(argv[0], argv[1], &charger_status)) {
    ble_elemind_charger_status_update(g_elemind, charger_status);
  }
}

/** Handle settings command.

    Update settings characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_settings(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get settings
  uint8_t settings = 0;
  if (parse_uint8_arg(argv[0], argv[1], &settings)) {
    ble_elemind_settings_update(g_elemind, settings);
  }
}

/** Handle memory level command.

    Update memory_level characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_memory_level(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get memory_level
  uint8_t memory_level = 0;
  if (parse_uint8_arg(argv[0], argv[1], &memory_level)) {
    ble_elemind_memory_level_update(g_elemind, memory_level);
  }
}

/** Handle factory reset command.

    Update factory_reset characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_factory_reset(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get factory_reset
  uint8_t factory_reset = 0;
  if (parse_uint8_arg(argv[0], argv[1], &factory_reset)) {
    ble_elemind_factory_reset_update(g_elemind, factory_reset);
  }
}

/** Handle sound control command.

    Update sound_control characteristic with value from command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
handle_sound_control(int argc, char **argv)
{
  if (argc != 2) {
    NRF_LOG_ERROR("Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get sound_control
  uint8_t sound_control = 0;
  if (parse_uint8_arg(argv[0], argv[1], &sound_control)) {
    ble_elemind_sound_control_update(g_elemind, sound_control);
  }
}

/** LPC command name to handler mapping table.

    Must be defined after handlers.
*/
struct lpc_command lpc_uart_commands[] = {
  {"ble_battery_level", handle_battery_level},
  {"ble_serial_number", handle_serial_number},
  {"ble_software_version", handle_software_version},
  {"ble_dfu", handle_ble_dfu},
  {"ble_electrode_quality", handle_electrode_quality},
  {"ble_volume", handle_volume},
  {"ble_power", handle_power},
  {"ble_therapy", handle_therapy},
  {"ble_heart_rate", handle_heart_rate},
  {"ble_blink_status", handle_blink_status},
  {"ble_quality_check", handle_quality_check},
  {"ble_alarm", handle_alarm},
  {"ble_sound", handle_sound},
  {"ble_time", handle_time},
  {"ble_charger_status", handle_charger_status},
  {"ble_settings", handle_settings},
  {"ble_memory_level", handle_memory_level},
  {"ble_factory_reset", handle_factory_reset},
  {"ble_sound_control", handle_sound_control},
};

/* Initialize the LPC UART interface. */
ret_code_t
lpc_uart_init(ble_bas_t* p_bas, ble_elemind_t* p_elemind)
{
  // Save battery service pointer for battery level updates from LPC
  g_bas_svc = p_bas;

  // Save Elemind service pointer for characteristic updates from LPC
  g_elemind = p_elemind;

  // Get BLE address
  ble_gap_addr_t ble_addr;
  sd_ble_gap_addr_get(&ble_addr);

  // Send BLE address to Processor
  // In case we want to use it to generate the serial or software version numbers.
  char buf_ptr[40];
  snprintf(buf_ptr, sizeof(buf_ptr), "ble_addr %02X%02X%02X%02X%02X%02X\n", 
  ble_addr.addr[5], ble_addr.addr[4], ble_addr.addr[3],
  ble_addr.addr[2], ble_addr.addr[1], ble_addr.addr[0]);
  lpc_uart_sendline(buf_ptr);

  // Send serial number and software version requests
  lpc_uart_sendline("ble_serial_number?\n");
  lpc_uart_sendline("ble_software_version?\n");
  lpc_uart_sendline("ble_settings?\n");
  lpc_uart_sendline("ble_memory_level?\n");

  return NRF_SUCCESS;
}

/* Send command or request to LPC. */
ret_code_t
lpc_uart_sendline(const char* p_line)
{
  NRF_LOG_INFO("Sending UART command: %s", p_line);

  bool success = bin_itf_send_command((char*)p_line, strlen(p_line));

  // ToDo: see if "NRF_ERROR_BUSY" is the correct error
  return success ? NRF_SUCCESS : NRF_ERROR_BUSY;
}

/* Send command with byte buffer parameters to LPC. */
ret_code_t
lpc_uart_send_buffer(uint8_t* buf, size_t buf_size)
{
  bool success = bin_itf_send_command((char*)buf, buf_size);

  // ToDo: see if "NRF_ERROR_BUSY" is the correct error
  return success ? NRF_SUCCESS : NRF_ERROR_BUSY;
}

/** Check if character is whitespace.

    @return true if character is whitespace, false otherwise
*/
static bool
is_whitespace(char c)
{
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

/** Find and run parsed command.

    @param argc Number of arguments (including command)
    @param argv Argument values
*/
static void
do_command(int argc, char **argv)
{
  unsigned int i;
  unsigned int nl = strlen(argv[0]);
  unsigned int cl;

  for (i = 0; i < ARRAY_SIZE(lpc_uart_commands); i++) {
    cl = strlen(lpc_uart_commands[i].name);

    if (cl == nl && lpc_uart_commands[i].function != NULL &&
      !strncmp(argv[0], lpc_uart_commands[i].name, nl)) {
      NRF_LOG_INFO("Running command: '%s'", lpc_uart_commands[i].name);
      lpc_uart_commands[i].function(argc, argv);
      break;
    }
  }

  if (i == ARRAY_SIZE(lpc_uart_commands)) {
    NRF_LOG_INFO("No command found for '%s'", argv[0]);
  }
}

/** Parse command line and execute command.

    @param command Command line
    @param length Command line length
*/
void
lpc_uart_parse_command(char* command, uint8_t length)
{
  unsigned char i;
  char *argv[ARGC_MAX];
  int argc = 0;
  char *in_arg = NULL;

  for (i = 0; (i < length) && (argc < ARRAY_SIZE(argv)); i++) {
    if (is_whitespace(command[i]) && argc == 0) {
      // Leading whitespace.
      continue;
    }

    if (is_whitespace(command[i])) {
      if (in_arg) {
        // Whitespace while in argument--this is the end of the arugment.
        command[i] = '\0';
        in_arg = NULL;
      }
    } else if (!in_arg) {
      /* Not whitespace, and not already in an argument--this is the
         start of the next argument. */
      in_arg = &command[i];
      argv[argc] = in_arg;
      argc++;
    }
  }
  command[i] = '\0';

  if (argc > 0) {
    do_command(argc, argv);
  }
}
