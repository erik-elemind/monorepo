#include "ble_uart_commands.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"

#include "loglevels.h"
#include "task.h"
#include "command_helpers.h"
#include "ymodem.h"
#include "eeg_reader.h"

#include "ble.h"
#include "hex_utils.h"


#define TAG "ble_uart_commands"

void
electrode_quality_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_electrode_quality_request();
}

void
volume_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_volume_request();
}

void
volume_command(int argc, char **argv)
{
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get volume
  uint8_t volume = 0;
  if (parse_uint8_arg(argv[0], argv[1], &volume)) {
    ble_volume_command(volume);
  }
}

void
power_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_power_request();
}

void
power_command(int argc, char **argv)
{
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get power
  uint8_t power = 0;
  if (parse_uint8_arg(argv[0], argv[1], &power)) {
    ble_power_command(power);
  }
}

void
therapy_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_therapy_request();
}

void
therapy_command(int argc, char **argv)
{
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get therapy
  uint8_t therapy = 0;
  if (parse_uint8_arg(argv[0], argv[1], &therapy)) {
    // Before starting therapy, make sure any file transfer is stopped.
    // This is possible via BLE only since the command to start therapy
    // can arrive via its own characteristic, bypassing the UART service.
    ymodem_end_session();
    ble_therapy_command(therapy);
  }
}

void
heart_rate_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_heart_rate_request();
}

void
battery_level_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_battery_level_request();
}

void
serial_number_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_serial_number_request();
}

void
software_version_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_software_version_request();
}


void
quality_check_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_quality_check_request();
}

void
quality_check_command(int argc, char **argv)
{
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get volume
  uint8_t quality_check = 0;
  if (parse_uint8_arg(argv[0], argv[1], &quality_check)) {
    ble_quality_check_command(quality_check);
  }
}


void
alarm_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_alarm_request();
}

void
alarm_command(int argc, char **argv)
{
  if (argc != 10) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  bool success = true;
  // Parse alarm
  alarm_params_t params = {.all={0}};
  uint8_t temp = 0;

  /// parse 'on'
  success &= parse_uint8_arg(argv[0], argv[1], &temp);
  params.flags.on = temp > 0;
  /// parse 'sun'
  success &= parse_uint8_arg(argv[0], argv[2], &temp);
  params.flags.sun = temp > 0;
  /// parse 'mon'
  success &= parse_uint8_arg(argv[0], argv[3], &temp);
  params.flags.mon = temp > 0;
  /// parse 'tue'
  success &= parse_uint8_arg(argv[0], argv[4], &temp);
  params.flags.tue = temp > 0;
  /// parse 'wed'
  success &= parse_uint8_arg(argv[0], argv[5], &temp);
  params.flags.wed = temp > 0;
  /// parse 'thu'
  success &= parse_uint8_arg(argv[0], argv[6], &temp);
  params.flags.thu = temp > 0;
  /// parse 'fri'
  success &= parse_uint8_arg(argv[0], argv[7], &temp);
  params.flags.fri = temp > 0;
  /// parse 'sat'
  success &= parse_uint8_arg(argv[0], argv[8], &temp);
  params.flags.sat = temp > 0;

  // parse time_sec
  uint32_t time_min = 0;
  success &= parse_uint32_arg(argv[0], argv[9], &time_min);
  params.minutes_after_midnight = (uint16_t) time_min;

  if (success) {
    ble_alarm_command(&params);
  }
}

void
sound_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_sound_request();
}

void
sound_command(int argc, char **argv)
{
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get therapy
  uint8_t sound = 0;
  if (parse_uint8_arg(argv[0], argv[1], &sound)) {
    ble_sound_command(sound);
  }
}

void
time_request(int argc, char **argv)
{
  if (argc != 1) {
    LOGE(TAG, "Error: Command '%s' takes no arguments", argv[0]);
    return;
  }

  ble_time_request();
}

void
time_command(int argc, char **argv)
{
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

  // Get time
  uint64_t time;
  if (parse_uint64_arg(argv[0], argv[1], &time)) {
    ble_time_command(time);
  }
}

void
addr_command(int argc, char **argv)
{
  if (argc != 2) {
    LOGE(TAG, "Error: Command '%s' missing argument", argv[0]);
    return;
  }

#define UINT8_SWAP(a,b) { uint8_t temp=(a);(a)=(b);(b)=temp; }
  // Set BLE address
  uint8_t addr[6];
  unsigned int outputLen = 0;
  int status = HexStringToBytes(argv[1], addr, sizeof(addr), &outputLen);
  if(status == 0 && outputLen == 6){
    // reverse the address, which is the convention used by nRF
    UINT8_SWAP(addr[0],addr[5]);
    UINT8_SWAP(addr[1],addr[4]);
    UINT8_SWAP(addr[2],addr[3]);
    ble_addr_command(addr);
  }else{
    LOGE(TAG, "Error: Command '%s' failed to parse argument %s", argv[0], argv[1]);
  }
#undef UINT8_SWAP
}

void
ble_print_addr_command(int argc, char **argv)
{
  uint8_t* addr = ble_get_addr();

  // Print BLE address in reverse order of the way it's stored, which is nRF convention.
  LOGV(TAG, "BLE Address: %02X%02X%02X%02X%02X%02X", addr[5], addr[4], addr[3],
      addr[2], addr[1], addr[0]);
}

void ble_connected(int argc, char **argv)
{
	LOGV(TAG, "BLE Connected");
	ble_connected_event();
}

void ble_disconnected(int argc, char **argv)
{
	LOGV(TAG, "BLE Disconnected");
	ble_disconnected_event();
}
