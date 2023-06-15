/*
 * pmic_commands.c
 *
 *  Created on: Dec 27, 2022
 *      Author: DavidWang
 */

#include "pmic_commands.h"
#include "pmic_pca9420.h"

void pmic_status_command(int argc, char **argv){
    pmic_dump_modes();
}

void pmic_test_command(int argc, char **argv){
	pmic_test();
}

void pmic_enter_ship_mode_command(int argc, char **argv)
{
	pmic_enter_ship_mode();
}

void pmic_batt_status(int argc, char **argv)
{
  // Print top-level status from driver
  printf("Battery status: ");
  battery_charger_status_t battery_charger_status = pmic_battery_charger_get_status();

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
  status_t status = pmic_battery_charger_print_detailed_status();

  if (status != kStatus_Success) {
    printf("Error printing detailed status: %ld (0x%lx)\n", status, status);
  }
}


