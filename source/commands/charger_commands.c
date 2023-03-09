#include "battery_charger.h"
#include <stdio.h>
#include "command_helpers.h"

extern battery_charger_handle_t g_battery_charger_handle;

void bq_charge_enable(int argc, char **argv)
{
  status_t status = battery_charger_enable(&g_battery_charger_handle, true);
  if (status == kStatus_Success) {
    printf("Battery charging enabled.\n");
  }
  else {
    printf("Error: %ld (0x%lx)\n", status, status);
  }
}

void bq_charge_disable(int argc, char **argv)
{
  status_t status = battery_charger_enable(&g_battery_charger_handle, false);
  if (status == kStatus_Success) {
    printf("Battery charging disabled.\n");
  }
  else {
    printf("Error: %ld (0x%lx)\n", status, status);
  }
}

void bq_status(int argc, char **argv)
{
  // Print top-level status from driver
  printf("Battery status: ");
  battery_charger_status_t battery_charger_status = battery_charger_get_status(
    &g_battery_charger_handle);
  switch (battery_charger_status) {
    case BATTERY_CHARGER_STATUS_ON_BATTERY:
      printf("ON_BATTERY");
      break;

    case BATTERY_CHARGER_STATUS_CHARGING:
      printf("CHARGING");
      break;

    case BATTERY_CHARGER_STATUS_CHARGE_COMPLETE:
      printf("CHARGE_COMPLETE");
      break;

    case BATTERY_CHARGER_STATUS_FAULT:
      printf("FAULT");
      break;

    default:
      printf("Error: Unknown status value");
      break;
  }
  printf("\n");

  // Ask driver to print detailed status itself
  status_t status = battery_charger_print_detailed_status(
    &g_battery_charger_handle);
  if (status != kStatus_Success) {
    printf("Error printing detailed status: %ld (0x%lx)\n", status, status);
  }
}

void bq_adc_enable(int argc, char **argv)
{
	CHK_ARGC(2,2);

	uint8_t enable = 0;
	if (parse_uint8_arg_max(argv[0], argv[1], 1, &enable)) {
		battery_charger_set_adc_enable(&g_battery_charger_handle, enable);
	}
}
