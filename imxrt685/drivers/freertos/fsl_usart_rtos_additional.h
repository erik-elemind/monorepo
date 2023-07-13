/*
 * fsl_usart_rtos2.h
 *
 *  Created on: Sep 5, 2021
 *      Author: DavidWang
 */

#ifndef FSL_USART_RTOS_ADDITIONAL_H_
#define FSL_USART_RTOS_ADDITIONAL_H_

#include "fsl_usart.h"
#include "fsl_usart_freertos.h"

#ifdef __cplusplus
extern "C" {
#endif

int USART_RTOS_Init_FlowControl(usart_rtos_handle_t *handle, usart_handle_t *t_handle, const struct rtos_usart_config *cfg, bool flowcontrol);

#ifdef __cplusplus
}
#endif


#endif /* FSL_USART_RTOS_ADDITIONAL_H_ */
