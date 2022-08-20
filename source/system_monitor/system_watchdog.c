
#include "system_watchdog.h"
#include "fsl_wwdt.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "timers.h"
//#include "queue.h"

#include "utils.h"
#include "config.h"


#define APP_WDT_IRQn        WDT_BOD_IRQn
#define APP_WDT_IRQ_HANDLER WDT_BOD_IRQHandler
#define WDT_CLK_FREQ        CLOCK_GetWdtClkFreq(0)


void APP_WDT_IRQ_HANDLER(void)
{
    uint32_t wdtStatus = WWDT_GetStatusFlags(WWDT0);

//    APP_LED_TOGGLE;

    /* The chip will reset before this happens */
    if (wdtStatus & kWWDT_TimeoutFlag)
    {
        WWDT_ClearStatusFlags(WWDT0, kWWDT_TimeoutFlag);
    }

    /* Handle warning interrupt */
    if (wdtStatus & kWWDT_WarningFlag)
    {
        /* A watchdog feed didn't occur prior to warning timeout */
        WWDT_ClearStatusFlags(WWDT0, kWWDT_WarningFlag);
        /* User code. User can do urgent case before timeout reset.
         * IE. user can backup the ram data or ram log to flash.
         * the period is set by config.warningValue, user need to
         * check the period between warning interrupt and timeout.
         */
    }
    SDK_ISR_EXIT_BARRIER;
}


void system_watchdog_init(){
  // setup watchdog timer
  wwdt_config_t config;
  uint32_t wdtFreq;

//  /* Enable FRO 1M clock for WWDT module. */
//  SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;
//  /* Set clock divider for WWDT clock source. */
//  CLOCK_SetClkDiv(kCLOCK_DivWdtClk, 1U, true);

  WWDT_GetDefaultConfig(&config);

  /* The WDT divides the input frequency into it by 4 */
  wdtFreq = WDT_CLK_FREQ / 4;

  /*
   * Set watchdog feed time constant to approximately 120s
   * Set watchdog warning time to 512 ticks after feed time constant
   * Set watchdog window time to 1s
   */
  config.timeoutValue = wdtFreq * 120;
  config.warningValue = 512;
//  config.windowValue  = wdtFreq * 1;
  config.windowValue  = 0xFFFFFF; // disable reset on early feed
  /* Configure WWDT to reset on timeout */
  config.enableWatchdogReset = true;
  /* Setup watchdog clock frequency(Hz). */
  config.clockFreq_Hz = WDT_CLK_FREQ;
  WWDT_Init(WWDT0, &config);

  NVIC_EnableIRQ(APP_WDT_IRQn);
}

void system_watchdog_feed(){
  WWDT_Refresh(WWDT0);
}


uint32_t system_watchdog_reset_ms(){
  return 5000;
}


