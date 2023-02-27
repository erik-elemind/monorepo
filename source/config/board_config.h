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
#elif defined(VARIANT_FF4)
  #include "board_ff4.h"
#else
  #error "Invalid variant"
#endif

/** Toggle Debug LED
 */
void
BOARD_ToggleDebugLED(void);

AT_QUICKACCESS_SECTION_CODE(void BOARD_SetDeepSleepPinConfig(void));
AT_QUICKACCESS_SECTION_CODE(void BOARD_RestoreDeepSleepPinConfig(void));
AT_QUICKACCESS_SECTION_CODE(void BOARD_EnterDeepSleep(const uint32_t exclude_from_pd[4]));
AT_QUICKACCESS_SECTION_CODE(void BOARD_EnterDeepPowerDown(const uint32_t exclude_from_pd[4]));

#ifdef __cplusplus
}
#endif

#endif  // BOARD_CONFIG_H
