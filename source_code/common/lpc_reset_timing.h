/*
 * lpc_reset_timing.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Bradey Honsinger / Paul Adelsbach
 *
 * Description: Define the timing values for resetting the LPC.
 *
 */
#pragma once

/** LPC RESETn hold time (in microseconds). */
#define LPC_RESETN_HOLD_US 100

/** LPC ISPn hold time after reset (in milliseconds). */
#define LPC_ISPN_HOLD_TIME_MS 10
