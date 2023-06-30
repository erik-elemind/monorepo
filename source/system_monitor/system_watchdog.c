
#include "system_watchdog.h"
#include "fsl_wwdt.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "timers.h"
//#include "queue.h"

#include "utils.h"
#include "config.h"


#define APP_WDT_IRQn        WDT0_IRQn
#define WDT_CLK_FREQ        CLOCK_GetWdtClkFreq(0)


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
   * Set watchdog pet time constant to approximately 30s
   * Set watchdog warning time to 512 ticks after feed time constant
   * Set watchdog window time to 1s
   */
  config.timeoutValue = wdtFreq * 30; //TODO: adjust accordingly 
  config.warningValue = 512;
//  config.windowValue  = wdtFreq * 1;
  config.windowValue  = 0xFFFFFF; // disable reset on early feed
  /* Configure WWDT to not reset on timeout for Memfault handling instead*/
  config.enableWatchdogReset = false;
  /* Setup watchdog clock frequency(Hz). */
  config.clockFreq_Hz = WDT_CLK_FREQ;
  WWDT_Init(WWDT0, &config);

  NVIC_EnableIRQ(APP_WDT_IRQn);
}

void system_watchdog_pet(){
  WWDT_Refresh(WWDT0);
}


