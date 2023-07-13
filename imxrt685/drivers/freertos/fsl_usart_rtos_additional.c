/*
 * fsl_usart_rtos2.c
 *
 *  Created on: Sep 5, 2021
 *      Author: DavidWang
 */


/*
 * The following code is copied from "fsl_usart_freertos.c",
 * USART_RTOS_Init_FlowControl is a modified version of USART_RTOS_Init that takes whether to enable flow control as a parameter.
 */

#include "fsl_usart_rtos_additional.h"

static void USART_RTOS_Callback(USART_Type *base, usart_handle_t *state, status_t status, void *param)
{
    usart_rtos_handle_t *handle = (usart_rtos_handle_t *)param;
    BaseType_t xHigherPriorityTaskWoken, xResult;

    xHigherPriorityTaskWoken = pdFALSE;
    xResult                  = pdFAIL;

    if (status == kStatus_USART_RxIdle)
    {
        xResult = xEventGroupSetBitsFromISR(handle->rxEvent, RTOS_USART_COMPLETE, &xHigherPriorityTaskWoken);
    }
    else if (status == kStatus_USART_TxIdle)
    {
        xResult = xEventGroupSetBitsFromISR(handle->txEvent, RTOS_USART_COMPLETE, &xHigherPriorityTaskWoken);
    }
    else if (status == kStatus_USART_RxRingBufferOverrun)
    {
        xResult = xEventGroupSetBitsFromISR(handle->rxEvent, RTOS_USART_RING_BUFFER_OVERRUN, &xHigherPriorityTaskWoken);
    }
    else
    {
        xResult = pdFAIL;
    }

    if (xResult != pdFAIL)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


/*FUNCTION**********************************************************************
 *
 * Function Name : USART_RTOS_Init
 * Description   : Initializes the USART instance for application
 *
 *END**************************************************************************/
/*!
 * brief Initializes a USART instance for operation in RTOS.
 *
 * param handle The RTOS USART handle, the pointer to allocated space for RTOS context.
 * param t_handle The pointer to allocated space where to store transactional layer internal state.
 * param cfg The pointer to the parameters required to configure the USART after initialization.
 * return kStatus_Success, others fail.
 */
int USART_RTOS_Init_FlowControl(usart_rtos_handle_t *handle, usart_handle_t *t_handle, const struct rtos_usart_config *cfg, bool flowcontrol)
{
    status_t status;
    usart_config_t defcfg;

    if (NULL == handle)
    {
        return kStatus_InvalidArgument;
    }
    if (NULL == t_handle)
    {
        return kStatus_InvalidArgument;
    }
    if (NULL == cfg)
    {
        return kStatus_InvalidArgument;
    }
    if (NULL == cfg->base)
    {
        return kStatus_InvalidArgument;
    }
    if (0U == cfg->srcclk)
    {
        return kStatus_InvalidArgument;
    }
    if (0U == cfg->baudrate)
    {
        return kStatus_InvalidArgument;
    }

    handle->base    = cfg->base;
    handle->t_state = t_handle;

    handle->txSemaphore = xSemaphoreCreateMutex();
    if (NULL == handle->txSemaphore)
    {
        return kStatus_Fail;
    }
    handle->rxSemaphore = xSemaphoreCreateMutex();
    if (NULL == handle->rxSemaphore)
    {
        vSemaphoreDelete(handle->txSemaphore);
        return kStatus_Fail;
    }
    handle->txEvent = xEventGroupCreate();
    if (NULL == handle->txEvent)
    {
        vSemaphoreDelete(handle->rxSemaphore);
        vSemaphoreDelete(handle->txSemaphore);
        return kStatus_Fail;
    }
    handle->rxEvent = xEventGroupCreate();
    if (NULL == handle->rxEvent)
    {
        vEventGroupDelete(handle->txEvent);
        vSemaphoreDelete(handle->rxSemaphore);
        vSemaphoreDelete(handle->txSemaphore);
        return kStatus_Fail;
    }
    USART_GetDefaultConfig(&defcfg);

    defcfg.baudRate_Bps = cfg->baudrate;
    defcfg.parityMode   = cfg->parity;
    defcfg.enableTx     = true;
    defcfg.enableRx     = true;

    /* Enable hardware flow control - which was not in the original driver*/
    defcfg.enableHardwareFlowControl = flowcontrol;

    status = USART_Init(handle->base, &defcfg, cfg->srcclk);
    if (status != kStatus_Success)
    {
        vEventGroupDelete(handle->rxEvent);
        vEventGroupDelete(handle->txEvent);
        vSemaphoreDelete(handle->rxSemaphore);
        vSemaphoreDelete(handle->txSemaphore);
        return kStatus_Fail;
    }
    status = USART_TransferCreateHandle(handle->base, handle->t_state, USART_RTOS_Callback, handle);
    if (status != kStatus_Success)
    {
        vEventGroupDelete(handle->rxEvent);
        vEventGroupDelete(handle->txEvent);
        vSemaphoreDelete(handle->rxSemaphore);
        vSemaphoreDelete(handle->txSemaphore);
        return kStatus_Fail;
    }
    USART_TransferStartRingBuffer(handle->base, handle->t_state, cfg->buffer, cfg->buffer_size);
    return kStatus_Success;
}




