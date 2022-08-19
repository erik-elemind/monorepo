/*
 * config_tracealyzer_isr.h
 *
 *  Created on: Jul 26, 2021
 *      Author: DavidWang
 */

#ifndef CONFIG_CONFIG_TRACEALYZER_ISR_H_
#define CONFIG_CONFIG_TRACEALYZER_ISR_H_

#include "config_tracealyzer.h"
#include "trcRecorder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EEG_DMA_RX_COMPLETE_ISR_TRACE eeg_dma_rx_complete_isr_trace
#define FLASH_SPI_ISR_TRACE flash_spi_isr_trace
#define AUDIO_I2S_ISR_TRACE audio_i2s_isr_trace
#define GINT0_ISR_TRACE gint0_isr_trace
#define HRM_PINT_ISR_TRACE hrm_pint_isr_trace
#define ACCEL_PINT_ISR_TRACE accel_pint_isr_trace
#define ALS_PINT_ISR_TRACE als_pint_isr_trace
#define CHARGER_PINT_ISR_TRACE charger_pint_isr_trace
#define EEG_DRDY_PINT_ISR_TRACE eeg_drdy_pint_isr_trace
#define GINT1_ISR_TRACE gint1_isr_trace
#define SYSTEM_MONITOR_CHARGER_TIMEOUT_ISR_TRACE system_monitor_charger_timeout_isr_trace

extern traceHandle EEG_DMA_RX_COMPLETE_ISR_TRACE;
extern traceHandle FLASH_SPI_ISR_TRACE;
extern traceHandle AUDIO_I2S_ISR_TRACE;
extern traceHandle GINT0_ISR_TRACE;
extern traceHandle HRM_PINT_ISR_TRACE;
extern traceHandle ACCEL_PINT_ISR_TRACE;
extern traceHandle ALS_PINT_ISR_TRACE;
extern traceHandle CHARGER_PINT_ISR_TRACE;
extern traceHandle EEG_DRDY_PINT_ISR_TRACE;
extern traceHandle GINT1_ISR_TRACE;
extern traceHandle SYSTEM_MONITOR_CHARGER_TIMEOUT_ISR_TRACE;

void init_tracealyzer_isr();


#if (defined(ENABLE_TRACEALYZER) && (ENABLE_TRACEALYZER > 0U)) &&\
    (defined(ENABLE_TRACEALYZER_ISR_EEG) && (ENABLE_TRACEALYZER_ISR_EEG > 0U))
#define TRACEALYZER_ISR_EEG_BEGIN(x) {{\
  vTraceStoreISRBegin(x);\
}}
#define TRACEALYZER_ISR_EEG_END(x) ({\
  vTraceStoreISREnd(x);\
})
#else
#define TRACEALYZER_ISR_EEG_BEGIN(x)
#define TRACEALYZER_ISR_EEG_END(x)
#endif


#if (defined(ENABLE_TRACEALYZER) && (ENABLE_TRACEALYZER > 0U)) &&\
    (defined(ENABLE_TRACEALYZER_ISR_FLASH) && (ENABLE_TRACEALYZER_ISR_FLASH > 0U))
#define TRACEALYZER_ISR_FLASH_BEGIN(x) {{\
  vTraceStoreISRBegin(x);\
}}
#define TRACEALYZER_ISR_FLASH_END(x) ({\
  vTraceStoreISREnd(x);\
})
#else
#define TRACEALYZER_ISR_FLASH_BEGIN(x)
#define TRACEALYZER_ISR_FLASH_END(x)
#endif


#if (defined(ENABLE_TRACEALYZER) && (ENABLE_TRACEALYZER > 0U)) &&\
    (defined(ENABLE_TRACEALYZER_ISR_AUDIO) && (ENABLE_TRACEALYZER_ISR_AUDIO > 0U))
#define TRACEALYZER_ISR_AUDIO_BEGIN(x) {{\
  vTraceStoreISRBegin(x);\
}}
#define TRACEALYZER_ISR_AUDIO_END(x) ({\
  vTraceStoreISREnd(x);\
})
#else
#define TRACEALYZER_ISR_AUDIO_BEGIN(x)
#define TRACEALYZER_ISR_AUDIO_END(x)
#endif


#if (defined(ENABLE_TRACEALYZER_ISR_OTHER) && (ENABLE_TRACEALYZER_ISR_OTHER > 0U))
#define TRACEALYZER_ISR_BEGIN(x) {{\
  vTraceStoreISRBegin(x);\
}}
#define TRACEALYZER_ISR_END(x) ({\
  vTraceStoreISREnd(x);\
})
#else
#define TRACEALYZER_ISR_BEGIN(x)
#define TRACEALYZER_ISR_END(x)
#endif




#ifdef __cplusplus
}
#endif

#endif /* CONFIG_CONFIG_TRACEALYZER_ISR_H_ */
