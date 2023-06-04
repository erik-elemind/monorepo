#include "battery_charger.h"
#include <stdio.h>
#include "command_helpers.h"


void batt_charge_enable(int argc, char **argv)
{
  status_t status = battery_charger_enable(true);
  if (status == kStatus_Success) {
    printf("Battery charging enabled.\n");
  }
  else {
    printf("Error: %ld (0x%lx)\n", status, status);
  }
}

void batt_charge_disable(int argc, char **argv)
{
  status_t status = battery_charger_enable(false);
  if (status == kStatus_Success) {
    printf("Battery charging disabled.\n");
  }
  else {
    printf("Error: %ld (0x%lx)\n", status, status);
  }
}

void batt_status(int argc, char **argv)
{
  // Print top-level status from driver
  printf("Battery status: ");
  battery_charger_status_t battery_charger_status = battery_charger_get_status();

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
  status_t status = battery_charger_print_detailed_status();

  if (status != kStatus_Success) {
    printf("Error printing detailed status: %ld (0x%lx)\n", status, status);
  }
}
