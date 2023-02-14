/*
 * ble_debug_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for BLE interface.
 *
 */
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "fsl_gpio.h"

#include "ble.h"
#include "board_config.h"
#include "command_helpers.h"
#include "binary_interface_inst.h"

#include "ble_debug_commands.h"

void
ble_ping_command(int argc, char **argv)
{
  char cbuf[10];
  size_t cbuf_size;

  cbuf_size = snprintf(cbuf,sizeof(cbuf),"pong\n");
  bin_itf_send_uart_command(cbuf,cbuf_size);
}


void
ble_reset_debug_command(int argc, char **argv)
{
  CHK_ARGC(1,1);

  ble_reset();

  printf("Reset BLE");
}

void
ble_dfu_debug_command(int argc, char **argv)
{
  CHK_ARGC(1,1);

  // Send DFU request to BLE task
  ble_dfu_request();

  printf("Sent DFU command");
}

void
ble_electrode_quality_debug_command(int argc, char **argv)
{
  CHK_ARGC(2,9);

  bool success = true;
  uint8_t electrode_quality[ELECTRODE_NUM] = { 0 };
  for (int i = 0; (i < (argc-1)) && success; i++) {
    success = parse_uint8_arg(argv[0], argv[i+1], &electrode_quality[i]);
  }

  if (success) {
    ble_electrode_quality_update(electrode_quality);
  }
}

void
ble_blink_status_debug_command(int argc, char **argv)
{
  if ((argc < 2) || (argc > 11)) {
    printf("Error: Incorrect number of arguments\n");
    printf("Usage: %s <1> [<2>..<10>]\n", argv[0]);
    return;
  }

  bool success = true;
  uint8_t blink_status[BLINK_NUM] = { 0 };
  for (int i = 0; (i < (argc-1)) && success; i++) {
    success = parse_uint8_arg(argv[0], argv[i+1], &blink_status[i]);
  }

  if (success) {
    ble_blink_status_update(blink_status);
  }
}


void
ble_volume_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint8_t volume = 0;
 if (parse_uint8_arg(argv[0], argv[1], &volume)) {
   ble_volume_update(volume);
 }
}

void
ble_power_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint8_t power = 0;
 if (parse_uint8_arg_max(argv[0], argv[1], 1, &power)) {
   ble_power_update(power);
 }
}

void
ble_therapy_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint8_t therapy = 0;
 if (parse_uint8_arg_max(argv[0], argv[1], 1, &therapy)) {
   ble_therapy_update(therapy);
 }
}

void
ble_heart_rate_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint8_t heart_rate = 0;
 if (parse_uint8_arg(argv[0], argv[1], &heart_rate)) {
   ble_heart_rate_update(heart_rate);
 }
}

void
ble_battery_level_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint8_t battery_level = 0;
 if (parse_uint8_arg_max(argv[0], argv[1], 100, &battery_level)) {
   ble_battery_level_update(battery_level);
 }
}

void
ble_quality_check_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint8_t quality_check = 0;
 if (parse_uint8_arg(argv[0], argv[1], &quality_check)) {
   ble_quality_check_update(quality_check);
 }
}

#include "loglevels.h"

void
ble_alarm_debug_command(int argc, char **argv)
{
 CHK_ARGC(10,10);

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

 // parse time_min
 uint32_t time_min = 0;
 success &= parse_uint32_arg(argv[0], argv[9], &time_min);
 params.minutes_after_midnight = (uint16_t) time_min;

 if (success) {
   ble_alarm_update(&params);
 }
}

void
ble_sound_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint8_t sound;
 if (parse_uint8_arg(argv[0], argv[1], &sound)) {
   ble_sound_update(sound);
 }
}

void
ble_time_debug_command(int argc, char **argv)
{
 CHK_ARGC(2,2);

 uint64_t time;
 if (parse_uint64_arg(argv[0], argv[1], &time)) {
   ble_time_update(time);
 }
}

void ble_charger_status_debug_command(int argc, char **argv)
{
	CHK_ARGC(2,2);

	uint8_t charger_status;
	if (parse_uint8_arg(argv[0], argv[1], &charger_status)) {
		ble_charger_status_update(charger_status);
	}
}

void ble_settings_debug_command(int argc, char **argv)
{
	CHK_ARGC(2,2);

	uint8_t settings;
	if (parse_uint8_arg(argv[0], argv[1], &settings)) {
		ble_settings_update(settings);
	}
}

void ble_memory_level_debug_command(int argc, char **argv)
{
	CHK_ARGC(2,2);

	uint8_t memory_level;
	if (parse_uint8_arg(argv[0], argv[1], &memory_level)) {
		ble_memory_level_update(memory_level);
	}
}

void ble_factory_reset_debug_command(int argc, char **argv)
{
	CHK_ARGC(2,2);

	uint8_t factory_reset;
	if (parse_uint8_arg(argv[0], argv[1], &factory_reset)) {
		ble_factory_reset_update(factory_reset);
	}
}

void ble_sound_control_debug_command(int argc, char **argv)
{
	CHK_ARGC(2,2);

	uint8_t sound_control;
	if (parse_uint8_arg(argv[0], argv[1], &sound_control)) {
		ble_sound_control_update(sound_control);
	}
}

