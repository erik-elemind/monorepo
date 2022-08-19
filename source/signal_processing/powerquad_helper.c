/*
 * powerquad_helper.c
 *
 *  Created on: Feb 27, 2022
 *      Author: DavidWang
 */

#include "powerquad_helper.h"
#include "config.h"

#include "fsl_powerquad.h"

#define POWERQUAD_INTERRUPT_PRIORITY (3U)

#if (defined(ENABLE_POWERQUAD_INTERRUPT) && (ENABLE_POWERQUAD_INTERRUPT > 0U))
static SemaphoreHandle_t xSemaphore = NULL;
static StaticSemaphore_t xSemaphoreBuffer;
#endif

 // copied from "fsl_powerquad.h"
#define PQ_SET_FFT_Q31_CONFIG                                                                         \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | (uint32_t)kPQ_32Bit; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | (uint32_t)kPQ_32Bit; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | (uint32_t)kPQ_32Bit; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | (uint32_t)kPQ_32Bit; \
    POWERQUAD->TMPBASE   = 0xE0000000U


void pqhelper_arm_rfft_q31(const arm_rfft_instance_q31 *S, q31_t *pSrc, q31_t *pDst)
{
    uint32_t length = S->fftLenReal;
    PQ_SET_FFT_Q31_CONFIG;

    /* Calculation of RFFT of input */
    PQ_TransformRFFT(POWERQUAD, length, pSrc, pDst);

#if (defined(ENABLE_POWERQUAD_INTERRUPT) && (ENABLE_POWERQUAD_INTERRUPT > 0U))
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
#else
    PQ_WaitDone(POWERQUAD);
#endif
}


void pqhelper_init(){

#if (defined(ENABLE_POWERQUAD_INTERRUPT) && (ENABLE_POWERQUAD_INTERRUPT > 0U))
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_PowerQuad);
#endif
#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    RESET_PeripheralReset(kPOWERQUAD_RST_SHIFT_RSTn);
#endif
  /* Enable event used for WFE. */
  POWERQUAD->EVENTEN = POWERQUAD_EVENTEN_EVENT_OFLOW_MASK | POWERQUAD_EVENTEN_EVENT_NAN_MASK |
                    POWERQUAD_EVENTEN_EVENT_FIXED_MASK | POWERQUAD_EVENTEN_EVENT_UFLOW_MASK |
                    POWERQUAD_EVENTEN_EVENT_BERR_MASK | POWERQUAD_EVENTEN_EVENT_COMP_MASK;

  // Trying to use PowerQuad Interrupt, but it doesn't work.
  /* Enable event used for WFE. */
  POWERQUAD->INTREN = POWERQUAD_INTREN_INTR_OFLOW_MASK | POWERQUAD_INTREN_INTR_NAN_MASK |
                  POWERQUAD_INTREN_INTR_FIXED_MASK | POWERQUAD_INTREN_INTR_UFLOW_MASK |
                  POWERQUAD_INTREN_INTR_BERR_MASK | POWERQUAD_INTREN_INTR_COMP_MASK;

  /* Interrupt vector GINT0_IRQn priority settings in the NVIC */
  NVIC_SetPriority(PQ_IRQn, POWERQUAD_INTERRUPT_PRIORITY);
  EnableIRQ(PQ_IRQn);

  /* Setup binary semaphore*/
  xSemaphore = xSemaphoreCreateBinaryStatic( &xSemaphoreBuffer );

#else
  // ToDo: Add power-saving logic to only have powerquad enabled when needed.
  // Enable clock for powerquad.
  PQ_Init(POWERQUAD);
#endif

}




#if ENABLE_POWERQUAD_INTERRUPT

#ifdef __cplusplus
extern "C" {
#endif

// Trying to use PowerQuad Interrupt, but it doesn't work.
void PQ_DriverIRQHandler(void);
void PQ_DriverIRQHandler(void)
{
  /* Clear interrupt before callback */
  POWERQUAD->INTRSTAT |= POWERQUAD_INTRSTAT_INTR_STAT_MASK;
  /* Call user function */
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
//  debug_uart_puts((char*)"PQinterrupt");
  SDK_ISR_EXIT_BARRIER
}

#ifdef __cplusplus
}
#endif

#endif



