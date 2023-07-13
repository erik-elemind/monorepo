/*
 * micro_clock.c
 *
 *  Created on: Nov 5, 2020
 *      Author: David Wang
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "micro_clock.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define CTIMER            CTIMER0
#define CTIMER_CLK_SRC    kSFRO_to_CTIMER0
#define CTIMER_MATCH      kCTIMER_Match_0
#define CTIMER_CLK_FREQ   (16000000U)
#define CTIMER_IRQn       CTIMER0_IRQn
#define CTIMER_INT_PRIO   MICRO_CLOCK_NVIC_PRIORITY // must be equal to or below min FreeRTOS priority.

#define MICRO_CLOCK_TICK_FREQ (1000000U) // 1Mhz tick to support 1 microsecond clock
#if (CTIMER_CLK_FREQ < MICRO_CLOCK_TICK_FREQ)
#error The clock frequency driving CTIMER0 must be equal to or greater than 1Mhz.
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void ctimer_overflow_callback(uint32_t flags);

/* Array of function pointers for callback for each channel */
ctimer_callback_t ctimer_callback_table[] = {ctimer_overflow_callback};

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Match Configuration for Channel 0 */
static ctimer_match_config_t g_match_config;
static ctimer_config_t g_ctimer_config;
static uint64_t g_total_micros = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void ctimer_overflow_callback(uint32_t flags)
{
  g_total_micros += (((uint64_t)0xFFFFFFFF) * MICRO_CLOCK_TICK_FREQ) / CTIMER_CLK_FREQ;
}


void init_micro_clock(void){
  CLOCK_AttachClk(CTIMER_CLK_SRC);                 /*!< Switch CTIMER0 to FRO1M */

  /* Get default ctimer configuration */
  // default = timer mode, no prescale.
  CTIMER_GetDefaultConfig(&g_ctimer_config);

  /* Init ctimer module */
  CTIMER_Init(CTIMER, &g_ctimer_config);

  /* Configuration 0 - overflow counter*/
  g_match_config.enableCounterReset = false;
  g_match_config.enableCounterStop  = false;
  g_match_config.matchValue         = 0xFFFFFFFF;
  g_match_config.outControl         = kCTIMER_Output_NoAction;
  g_match_config.outPinInitState    = false;
  g_match_config.enableInterrupt    = true;

  /* Interrupt vector CTIMER_IRQn priority settings in the NVIC. */
  NVIC_SetPriority(CTIMER_IRQn, CTIMER_INT_PRIO);

  CTIMER_RegisterCallBack(CTIMER, &ctimer_callback_table[0], kCTIMER_SingleCallback);

  CTIMER_SetupMatch(CTIMER, CTIMER_MATCH, &g_match_config);

  CTIMER_StartTimer(CTIMER);
}


void deinit_micro_clock(void){
  CTIMER_StopTimer(CTIMER);
  CTIMER_Deinit(CTIMER);
}


uint64_t micros(void){
  NVIC_DisableIRQ(CTIMER_IRQn);
  uint64_t total_micros =  (((uint64_t)CTIMER_GetTimerCountValue(CTIMER)) * MICRO_CLOCK_TICK_FREQ) / CTIMER_CLK_FREQ;
  total_micros = (g_total_micros + total_micros);
  NVIC_EnableIRQ(CTIMER_IRQn);
  return total_micros;
}

uint64_t millis(void){
  return micros() / 1000;
}



