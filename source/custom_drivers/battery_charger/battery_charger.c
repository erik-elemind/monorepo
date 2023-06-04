/*
 * battery_charger.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger, Tyler Gage, David Wang
 *
 * Description: PCA9420 PMIC Battery Charger driver implementation.
 */
#include "battery_charger.h"

#include "config.h"
#include "loglevels.h"


/// Logging prefix
static const char* TAG = "battery_charger";


// Global function definitions
void battery_charger_init(void)
{
  LOGV(TAG, "battery_charger_init");
}

status_t battery_charger_enable(bool enable)
{
  LOGV(TAG, "battery_charger_enable");
  return kStatus_Success;
}

bool battery_charger_is_enabled(void)
{
  return true;
}


battery_charger_status_t battery_charger_get_status(void)
{
  return BATTERY_CHARGER_STATUS_CHARGING;
}

status_t battery_charger_print_detailed_status(void)
{

  return kStatus_Success;
}
