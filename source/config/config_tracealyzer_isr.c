/*
 * config_tracealzyer.c
 *
 *  Created on: Jul 25, 2021
 *      Author: DavidWang
 */

#include "config_tracealyzer_isr.h"

#define PRIO_ISR_TRACE 5 /* the hardware priority level */


traceHandle EEG_DMA_RX_COMPLETE_ISR_TRACE;
traceHandle FLASH_SPI_ISR_TRACE;
traceHandle AUDIO_I2S_ISR_TRACE;
traceHandle GINT0_ISR_TRACE;
traceHandle HRM_PINT_ISR_TRACE;
traceHandle ACCEL_PINT_ISR_TRACE;
traceHandle ALS_PINT_ISR_TRACE;
traceHandle CHARGER_PINT_ISR_TRACE;
traceHandle EEG_DRDY_PINT_ISR_TRACE;
traceHandle GINT1_ISR_TRACE;
traceHandle SYSTEM_MONITOR_CHARGER_TIMEOUT_ISR_TRACE;


void init_tracealyzer_isr(){
  EEG_DMA_RX_COMPLETE_ISR_TRACE = xTraceSetISRProperties("eeg_dma_rx_complete_isr", PRIO_ISR_TRACE);
  FLASH_SPI_ISR_TRACE = xTraceSetISRProperties("flash_spi_isr", PRIO_ISR_TRACE);
  AUDIO_I2S_ISR_TRACE = xTraceSetISRProperties("audio_i2s_isr", PRIO_ISR_TRACE);
  GINT0_ISR_TRACE = xTraceSetISRProperties("gint0_isr", PRIO_ISR_TRACE);
  HRM_PINT_ISR_TRACE = xTraceSetISRProperties("hrm_pint_isr", PRIO_ISR_TRACE);
  ACCEL_PINT_ISR_TRACE = xTraceSetISRProperties("accel_pint_isr", PRIO_ISR_TRACE);
  ALS_PINT_ISR_TRACE = xTraceSetISRProperties("als_pint_isr", PRIO_ISR_TRACE);
  CHARGER_PINT_ISR_TRACE = xTraceSetISRProperties("charger_pint_isr", PRIO_ISR_TRACE);
  EEG_DRDY_PINT_ISR_TRACE = xTraceSetISRProperties("eeg_drdy_pint_isr", PRIO_ISR_TRACE);
  GINT1_ISR_TRACE = xTraceSetISRProperties("gint1_isr", PRIO_ISR_TRACE);
  SYSTEM_MONITOR_CHARGER_TIMEOUT_ISR_TRACE = xTraceSetISRProperties("charger_timeout_isr", PRIO_ISR_TRACE);
}


#if (defined(ENABLE_TRACEALYZER) && (ENABLE_TRACEALYZER > 0U))

#endif



