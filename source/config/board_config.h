/*
 * board_config.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Morpheus board configuration.
 *
 * Constants and functions that vary between boards. Note that most
 * constants are in board-specific header files; the correct
 * board-specific header files is included here.
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "prebuild.h"

#ifdef __cplusplus
extern "C" {
#endif

// VARIANT_**** defines MUST be selected in makefile.defs
#if defined(VARIANT_EVKS)
  #include "board_evks.h"
#elif defined(VARIANT_NFF1)
  #include "board_nff1.h"
#elif defined(VARIANT_FF1)
  #include "board_ff1.h"
#elif defined(VARIANT_FF2)
  #include "board_ff2.h"
#elif defined(VARIANT_FF3)
  #include "board_ff3.h"
#else
  #error "Invalid variant"
#endif

/** Set RGB LED to specified color.

    Sets LED color using three percentage values.

    @param red_duty_cycle_percent Percent brightness for red LED (0-100)
    @param green_duty_cycle_percent Percent brightness for green LED (0-100)
    @param blue_duty_cycle_percent Percent brightness for blue LED (0-100)
 */
void
BOARD_SetRGB(
  uint8_t red_duty_cycle_percent,
  uint8_t grn_duty_cycle_percent,
  uint8_t blu_duty_cycle_percent
  );

/** Set RGB LED to specified color using a single value for convenience.

    @param rgb_duty_cycle_bytes RGB value encoded as 0x00RRGGBB, i.e.
    red is byte 2, green is byte 1, blue is byte 0 (LSB). Note that
    RGB byte values (0-255) are scaled to percentage values (0-100)
    for display.
 */
void
BOARD_SetRGB32(uint32_t rgb_duty_cycle_bytes);

/** Toggle Debug LED
 */
void
BOARD_ToggleDebugLED(void);

#ifdef __cplusplus
}
#endif

#endif  // BOARD_CONFIG_H
