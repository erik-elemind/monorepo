/*
 * app_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for app task.
 *
 */
#include <stdbool.h>
#include <stdint.h>

#include "app.h"
#include "config.h"
//#include "utils.h"
#include "loglevels.h"
#include "command_helpers.h"


#include "app_commands.h"


void
app_event_command(int argc, char **argv) {

  if(argc != 2) {
    printf("Usage: %s <event>\n", argv[0]);
    printf("  Events:\n");
    printf("    power_button_down\n");
    printf("    power_button_up\n");
    printf("    power_button_click\n");
    printf("    power_button_double_click\n");
    printf("    power_button_long_click\n");
    printf("    ble_activity\n");
    printf("    button_activity\n");
    printf("    shell_activity\n");
    printf("    power_off_timeout\n");
    printf("    ble_off_timeout\n");
    printf("    charger_plugged\n");
    printf("    charger_unplugged\n");
    printf("    charge_complete\n");
    printf("    charge_fault\n");
    return;
  }

  if (!strcmp(argv[1], "power_button_click")) {
    app_event_power_button_click();
  }
  else if (!strcmp(argv[1], "power_button_double_click")) {
    app_event_power_button_double_click();
  }
  else if (!strcmp(argv[1], "power_button_long_click")) {
    app_event_power_button_long_click();
  }
  else if (!strcmp(argv[1], "ble_activity")) {
    app_event_ble_activity();
  }
  else if (!strcmp(argv[1], "button_activity")) {
    app_event_button_activity();
  }
  else if (!strcmp(argv[1], "shell_activity")) {
    app_event_shell_activity();
  }
  else if (!strcmp(argv[1], "power_off_timeout")) {
    app_event_power_off_timeout();
  }
  else if (!strcmp(argv[1], "ble_off_timeout")) {
    app_event_ble_off_timeout();
  }
  else if (!strcmp(argv[1], "charger_plugged")) {
    app_event_charger_plugged();
  }
  else if (!strcmp(argv[1], "charger_unplugged")) {
    app_event_charger_unplugged();
  }
  else if (!strcmp(argv[1], "charge_complete")) {
    app_event_charge_complete();
  }
  else if (!strcmp(argv[1], "charge_fault")) {
    app_event_charge_fault();
  }
  else {
    printf("Unknown event!\n");
  }

}
