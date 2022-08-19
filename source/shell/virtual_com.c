/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * This SDK CDC VCOM BM implementation was modified with changes described at this URL:
 * https://community.nxp.com/t5/Kinetis-Software-Development-Kit/How-to-use-CDC-VCOM-example-with-printf/m-p/632512#M6737
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_cdc_acm.h"
#include "usb_device_ch9.h"

#include "usb_device_descriptor.h"
#include "virtual_com.h"

#include "utils.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "loglevels.h"
#include "portmacro.h"

#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

#if ((defined FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U))
#include "usb_phy.h"
#endif
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
extern uint8_t USB_EnterLowpowerMode(void);
#endif
#include "fsl_power.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void BOARD_InitHardware(void);
void USB_DeviceClockInit(void);
void USB_DeviceIsrEnable(void);
#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle);
#endif

void BOARD_DbgConsole_Deinit(void);
void BOARD_DbgConsole_Init(void);

usb_status_t USB_DeviceCdcVcomCallback(class_handle_t handle, uint32_t event, void *param);
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);

/*******************************************************************************
 * Definitions
 ******************************************************************************/
//static const char *TAG = "vcom";  // Logging prefix for this module

/* Receive buffer size, in bytes. Holds 2 HS or 8 FS packets. */
#define RECV_STREAM_BUFFER_SIZE (2048+1)

/* Receive buffer trigger level. Since this is for interactive use, we
   want to trigger on a single byte. */
#define RECV_STREAM_BUFFER_TRIGGER_LEVEL 1

/*******************************************************************************
 * Variables
 ******************************************************************************/

static volatile bool g_sendFinished = 0;

SemaphoreHandle_t xSemaphore = NULL;
StaticSemaphore_t xMutexBuffer;

extern usb_device_endpoint_struct_t g_UsbDeviceCdcVcomDicEndpoints[];
extern usb_device_class_struct_t g_UsbDeviceCdcVcomConfig;

/* Data structure of virtual com device */
usb_cdc_vcom_struct_t s_cdcVcom;

/* Line coding of cdc device */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_lineCoding[LINE_CODING_SIZE] = {
    /* E.g. 0x00,0xC2,0x01,0x00 : 0x0001C200 is 115200 bits per second */
    (LINE_CODING_DTERATE >> 0U) & 0x000000FFU,
    (LINE_CODING_DTERATE >> 8U) & 0x000000FFU,
    (LINE_CODING_DTERATE >> 16U) & 0x000000FFU,
    (LINE_CODING_DTERATE >> 24U) & 0x000000FFU,
    LINE_CODING_CHARFORMAT,
    LINE_CODING_PARITYTYPE,
    LINE_CODING_DATABITS};

/* Abstract state of cdc device */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_abstractState[COMM_FEATURE_DATA_SIZE] = {(STATUS_ABSTRACT_STATE >> 0U) & 0x00FFU,
                                                          (STATUS_ABSTRACT_STATE >> 8U) & 0x00FFU};

/* Country code of cdc device */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_countryCode[COMM_FEATURE_DATA_SIZE] = {(COUNTRY_SETTING >> 0U) & 0x00FFU,
                                                        (COUNTRY_SETTING >> 8U) & 0x00FFU};

/* CDC ACM information */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) static usb_cdc_acm_info_t s_usbCdcAcmInfo;
/* Data buffer for receiving and sending*/
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) static uint8_t s_currRecvBuf[DATA_BUFF_SIZE];
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) static uint8_t s_currSendBuf[DATA_BUFF_SIZE];
volatile static uint32_t s_recvSize    = 0;
volatile static uint32_t s_sendSize    = 0;

static uint8_t s_recvStreamArray[ RECV_STREAM_BUFFER_SIZE ];
static StaticStreamBuffer_t s_recvStreamStruct;
static StreamBufferHandle_t s_recvStreamBuffer = NULL;

/* USB device class information */
static usb_device_class_config_struct_t s_cdcAcmConfig[1] = {{
    USB_DeviceCdcVcomCallback,
    0,
    &g_UsbDeviceCdcVcomConfig,
}};

/* USB device class configuration information */
static usb_device_class_config_list_struct_t s_cdcAcmConfigList = {
    s_cdcAcmConfig,
    USB_DeviceCallback,
    1,
};

//static uint32_t s_usbBulkMaxPacketSize = HS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
volatile static uint8_t s_waitForDataReceive = 0;
volatile static uint8_t s_comOpen            = 0;
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
#if (defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U))
void USB0_IRQHandler(void)
{
    USB_DeviceLpcIp3511IsrFunction(s_cdcVcom.deviceHandle);
}
#endif
#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
void USB1_IRQHandler(void)
{
    USB_DeviceLpcIp3511IsrFunction(s_cdcVcom.deviceHandle);
}
#endif
/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL     (0x05U)
#define BOARD_USB_PHY_TXCAL45DP (0x0AU)
#define BOARD_USB_PHY_TXCAL45DM (0x0AU)
void USB_DeviceClockInit(void)
{
#if defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };
#endif

#if defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)
    /* enable USB IP clock */
    CLOCK_EnableUsbfs0DeviceClock(kCLOCK_UsbfsSrcFro, CLOCK_GetFroHfFreq());
#if defined(FSL_FEATURE_USB_USB_RAM) && (FSL_FEATURE_USB_USB_RAM)
    for (int i = 0; i < FSL_FEATURE_USB_USB_RAM; i++)
    {
        ((uint8_t *)FSL_FEATURE_USB_USB_RAM_BASE_ADDRESS)[i] = 0x00U;
    }
#endif

#endif
#if defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)
    /* enable USB IP clock */
    CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_UsbPhySrcExt, BOARD_XTAL0_CLK_HZ);
    CLOCK_EnableUsbhs0DeviceClock(kCLOCK_UsbSrcUnused, 0U);
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
#if defined(FSL_FEATURE_USBHSD_USB_RAM) && (FSL_FEATURE_USBHSD_USB_RAM)
    for (int i = 0; i < FSL_FEATURE_USBHSD_USB_RAM; i++)
    {
        ((uint8_t *)FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS)[i] = 0x00U;
    }
#endif
#endif
}
void USB_DeviceIsrEnable(void)
{
    uint8_t irqNumber;
#if defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)
    uint8_t usbDeviceIP3511Irq[] = USB_IRQS;
    irqNumber                    = usbDeviceIP3511Irq[CONTROLLER_ID - kUSB_ControllerLpcIp3511Fs0];
#endif
#if defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)
    uint8_t usbDeviceIP3511Irq[] = USBHSD_IRQS;
    irqNumber                    = usbDeviceIP3511Irq[CONTROLLER_ID - kUSB_ControllerLpcIp3511Hs0];
#endif
    /* Install isr, set priority, and enable IRQ. */
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    EnableIRQ((IRQn_Type)irqNumber);
}
#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle)
{
#if defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)
    USB_DeviceLpcIp3511TaskFunction(deviceHandle);
#endif
#if defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)
    USB_DeviceLpcIp3511TaskFunction(deviceHandle);
#endif
}
#endif
/*!
 * @brief CDC class specific callback function.
 *
 * This function handles the CDC class specific requests.
 *
 * @param handle          The CDC ACM class handle.
 * @param event           The CDC ACM class event type.
 * @param param           The parameter of the class specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCdcVcomCallback(class_handle_t handle, uint32_t event, void *param)
{
//  debug_uart_puts( "  callback" );
#if ((defined USB_DEVICE_CONFIG_CDC_CIC_EP_DISABLE) && (USB_DEVICE_CONFIG_CDC_CIC_EP_DISABLE > 0U))
#else
    uint32_t len;
#endif
    uint8_t *uartBitmap;
    usb_device_cdc_acm_request_param_struct_t *acmReqParam;
    usb_device_endpoint_callback_message_struct_t *epCbParam;
    usb_status_t error          = kStatus_USB_InvalidRequest;
    usb_cdc_acm_info_t *acmInfo = &s_usbCdcAcmInfo;
    acmReqParam                 = (usb_device_cdc_acm_request_param_struct_t *)param;
    epCbParam                   = (usb_device_endpoint_callback_message_struct_t *)param;
    switch (event)
    {
        case kUSB_DeviceCdcEventSendResponse:
        {
            if ((epCbParam->length != 0) && (!(epCbParam->length % g_UsbDeviceCdcVcomDicEndpoints[0].maxPacketSize)))
            {
                /* If the last packet is the size of endpoint, then send also zero-ended packet,
                 ** meaning that we want to inform the host that we do not have any additional
                 ** data, so it can flush the output.
                 */
                error = USB_DeviceCdcAcmSend(handle, USB_CDC_VCOM_BULK_IN_ENDPOINT, NULL, 0);
            }
            else if ((1 == s_cdcVcom.attach) && (1 == s_cdcVcom.startTransactions))
            {
                if ((epCbParam->buffer != NULL) || ((epCbParam->buffer == NULL) && (epCbParam->length == 0)))
                {
//                  debug_uart_puts( "  sendFinished" );
                    /* User: add your own code for send complete event */
                    g_sendFinished = true;

#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                    s_waitForDataReceive = 1;
                    USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
#endif
                }
            }
            else
            {
            }
        }
        break;
        case kUSB_DeviceCdcEventRecvResponse:
        {
            BaseType_t higherPriorityTaskWoken = pdFALSE;

            if ((1 == s_cdcVcom.attach) && (1 == s_cdcVcom.startTransactions))
            {
                s_recvSize = epCbParam->length;
                error      = kStatus_USB_Success;

                // Copy received data into stream buffer
                size_t bytesSent = xStreamBufferSendFromISR(s_recvStreamBuffer,
                    epCbParam->buffer, epCbParam->length, &higherPriorityTaskWoken);

                if (bytesSent != epCbParam->length) {
                  // Not enough space in stream buffer for received data
                  // Note: Can't call LOGx() functions from ISR context (malloc in printf)
                  debug_uart_puts("rerr:");
                  debug_uart_puti(epCbParam->length);
                  debug_uart_puti(bytesSent);
                }
                else {
                  error = kStatus_USB_Success;
                }

#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                s_waitForDataReceive = 0;
                USB0->INTEN |= USB_INTEN_SOFTOKEN_MASK;
#endif

                    /* Schedule buffer for next receive event */
                    error = USB_DeviceCdcAcmRecv(handle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, s_currRecvBuf,
                                                 g_UsbDeviceCdcVcomDicEndpoints[1].maxPacketSize);
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                    s_waitForDataReceive = 1;
                    USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
#endif
            }
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
        break;
        case kUSB_DeviceCdcEventSerialStateNotif:
            ((usb_device_cdc_acm_struct_t *)handle)->hasSentState = 0;
            error                                                 = kStatus_USB_Success;
            break;
        case kUSB_DeviceCdcEventSendEncapsulatedCommand:
            break;
        case kUSB_DeviceCdcEventGetEncapsulatedResponse:
            break;
        case kUSB_DeviceCdcEventSetCommFeature:
            if (USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == acmReqParam->setupValue)
            {
                if (1 == acmReqParam->isSetup)
                {
                    *(acmReqParam->buffer) = s_abstractState;
                    *(acmReqParam->length) = sizeof(s_abstractState);
                }
                else
                {
                    /* no action, data phase, s_abstractState has been assigned */
                }
                error = kStatus_USB_Success;
            }
            else if (USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == acmReqParam->setupValue)
            {
                if (1 == acmReqParam->isSetup)
                {
                    *(acmReqParam->buffer) = s_countryCode;
                    *(acmReqParam->length) = sizeof(s_countryCode);
                }
                else
                {
                    /* no action, data phase, s_countryCode has been assigned */
                }
                error = kStatus_USB_Success;
            }
            else
            {
                /* no action, return kStatus_USB_InvalidRequest */
            }
            break;
        case kUSB_DeviceCdcEventGetCommFeature:
            if (USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == acmReqParam->setupValue)
            {
                *(acmReqParam->buffer) = s_abstractState;
                *(acmReqParam->length) = COMM_FEATURE_DATA_SIZE;
                error                  = kStatus_USB_Success;
            }
            else if (USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == acmReqParam->setupValue)
            {
                *(acmReqParam->buffer) = s_countryCode;
                *(acmReqParam->length) = COMM_FEATURE_DATA_SIZE;
                error                  = kStatus_USB_Success;
            }
            else
            {
                /* no action, return kStatus_USB_InvalidRequest */
            }
            break;
        case kUSB_DeviceCdcEventClearCommFeature:
            break;
        case kUSB_DeviceCdcEventGetLineCoding:
            *(acmReqParam->buffer) = s_lineCoding;
            *(acmReqParam->length) = LINE_CODING_SIZE;
            error                  = kStatus_USB_Success;
            break;
        case kUSB_DeviceCdcEventSetLineCoding:
        {
            if (1 == acmReqParam->isSetup)
            {
                *(acmReqParam->buffer) = s_lineCoding;
                *(acmReqParam->length) = sizeof(s_lineCoding);
            }
            else
            {
                /* no action, data phase, s_lineCoding has been assigned */
            }
            error = kStatus_USB_Success;
        }
        break;
        case kUSB_DeviceCdcEventSetControlLineState:
        {
            s_usbCdcAcmInfo.dteStatus = acmReqParam->setupValue;
            /* activate/deactivate Tx carrier */
            if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
            {
                acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
            }
            else
            {
                acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
            }

            /* activate carrier and DTE. Com port of terminal tool running on PC is open now */
            if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE)
            {
                acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
            }
            /* Com port of terminal tool running on PC is closed now */
            else
            {
                acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
            }

            /* Indicates to DCE if DTE is present or not */
            acmInfo->dtePresent = (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE) ? 1 : 0;

            /* Initialize the serial state buffer */
            acmInfo->serialStateBuf[0] = NOTIF_REQUEST_TYPE;                /* bmRequestType */
            acmInfo->serialStateBuf[1] = USB_DEVICE_CDC_NOTIF_SERIAL_STATE; /* bNotification */
            acmInfo->serialStateBuf[2] = 0x00;                              /* wValue */
            acmInfo->serialStateBuf[3] = 0x00;
            acmInfo->serialStateBuf[4] = 0x00; /* wIndex */
            acmInfo->serialStateBuf[5] = 0x00;
            acmInfo->serialStateBuf[6] = UART_BITMAP_SIZE; /* wLength */
            acmInfo->serialStateBuf[7] = 0x00;
            /* Notify to host the line state */
            acmInfo->serialStateBuf[4] = acmReqParam->interfaceIndex;
            /* Lower byte of UART BITMAP */
            uartBitmap    = (uint8_t *)&acmInfo->serialStateBuf[NOTIF_PACKET_SIZE + UART_BITMAP_SIZE - 2];
            uartBitmap[0] = acmInfo->uartState & 0xFFu;
            uartBitmap[1] = (acmInfo->uartState >> 8) & 0xFFu;
#if ((defined USB_DEVICE_CONFIG_CDC_CIC_EP_DISABLE) && (USB_DEVICE_CONFIG_CDC_CIC_EP_DISABLE > 0U))
#else
            len = (uint32_t)(NOTIF_PACKET_SIZE + UART_BITMAP_SIZE);
            if (0 == ((usb_device_cdc_acm_struct_t *)handle)->hasSentState)
            {
                error = USB_DeviceCdcAcmSend(handle, USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT, acmInfo->serialStateBuf, len);
                if (kStatus_USB_Success != error)
                {
                    usb_echo("kUSB_DeviceCdcEventSetControlLineState error!");
                }
                ((usb_device_cdc_acm_struct_t *)handle)->hasSentState = 1;
            }
#endif
            /* Update status */
            if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
            {
                /*  To do: CARRIER_ACTIVATED */
            }
            else
            {
                /* To do: CARRIER_DEACTIVATED */
            }

            if (1 == s_cdcVcom.attach)
            {
              if( (acmInfo->uartState & USB_DEVICE_CDC_UART_STATE_RX_CARRIER) != 0){
                s_cdcVcom.startTransactions = 1;
              }else{
                s_cdcVcom.startTransactions = 0;
              }
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
                s_waitForDataReceive = 1;
                USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
                s_comOpen = 1;
                usb_echo("USB_APP_CDC_DTE_ACTIVATED\r\n");
#endif
            }
            error = kStatus_USB_Success;
        }
        break;
        case kUSB_DeviceCdcEventSendBreak:
            break;
        default:
            break;
    }

    return error;
}

/*!
 * @brief USB device callback function.
 *
 * This function handles the usb device specific requests.
 *
 * @param handle          The USB device handle.
 * @param event           The USB device event type.
 * @param param           The parameter of the device specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_InvalidRequest;
    uint16_t *temp16   = (uint16_t *)param;
    uint8_t *temp8     = (uint8_t *)param;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            s_cdcVcom.attach               = 0;
            s_cdcVcom.currentConfiguration = 0U;
            error                          = kStatus_USB_Success;
#if (defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)) || \
    (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
            /* Get USB speed to configure the device, including max packet size and interval of the endpoints. */
            if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &s_cdcVcom.speed))
            {
                USB_DeviceSetSpeed(handle, s_cdcVcom.speed);
            }
#endif
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (0U == (*temp8))
            {
                s_cdcVcom.attach               = 0;
                s_cdcVcom.currentConfiguration = 0U;
                error                          = kStatus_USB_Success;
            }
            else if (USB_CDC_VCOM_CONFIGURE_INDEX == (*temp8))
            {
                s_cdcVcom.attach               = 1;
                s_cdcVcom.currentConfiguration = *temp8;
                error                          = kStatus_USB_Success;
                /* Schedule buffer for receive */
                USB_DeviceCdcAcmRecv(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, s_currRecvBuf,
                                     g_UsbDeviceCdcVcomDicEndpoints[1].maxPacketSize);
                }
                else
                {
                /* no action, return kStatus_USB_InvalidRequest */
                }
            break;
        case kUSB_DeviceEventSetInterface:
            if (s_cdcVcom.attach)
            {
                uint8_t interface        = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
                if (interface == USB_CDC_VCOM_COMM_INTERFACE_INDEX)
                {
                    if (alternateSetting < USB_CDC_VCOM_COMM_INTERFACE_ALTERNATE_COUNT)
                {
                        s_cdcVcom.currentInterfaceAlternateSetting[interface] = alternateSetting;
                        error                                                 = kStatus_USB_Success;
                }
                }
                else if (interface == USB_CDC_VCOM_DATA_INTERFACE_INDEX)
                {
                    if (alternateSetting < USB_CDC_VCOM_DATA_INTERFACE_ALTERNATE_COUNT)
                {
                        s_cdcVcom.currentInterfaceAlternateSetting[interface] = alternateSetting;
                        error                                                 = kStatus_USB_Success;
                }
                }
                else
                {
                    /* no action, return kStatus_USB_InvalidRequest */
                }
                }
            break;
        case kUSB_DeviceEventGetConfiguration:
            if (param)
                {
                /* Get current configuration request */
                *temp8 = s_cdcVcom.currentConfiguration;
                error  = kStatus_USB_Success;
                }
            break;
        case kUSB_DeviceEventGetInterface:
            if (param)
            {
                /* Get current alternate setting of the interface request */
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                if (interface < USB_CDC_VCOM_INTERFACE_COUNT)
                {
                    *temp16 = (*temp16 & 0xFF00U) | s_cdcVcom.currentInterfaceAlternateSetting[interface];
                    error   = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (param)
            {
                error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (param)
            {
                error = USB_DeviceGetConfigurationDescriptor(handle,
                                                             (usb_device_get_configuration_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            if (param)
            {
                /* Get device string descriptor request */
                error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            }
            break;
        default:
            /* no action, return kStatus_USB_InvalidRequest */
            break;
    }

    return error;
}


/* Initialize virtual serial port. */
void virtual_com_init_helper(void)
{
    USB_DeviceClockInit();
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
  SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    s_cdcVcom.speed        = USB_SPEED_HIGH;//USB_SPEED_FULL;
    s_cdcVcom.attach       = 0;
    s_cdcVcom.cdcAcmHandle = (class_handle_t)NULL;
    s_cdcVcom.deviceHandle = NULL;

    if (kStatus_USB_Success != USB_DeviceClassInit(CONTROLLER_ID, &s_cdcAcmConfigList, &s_cdcVcom.deviceHandle))
    {
        usb_echo("USB device init failed\r\n");
    }
    else
    {
      usb_echo("USB device CDC virtual com demo\r\n");
        s_cdcVcom.cdcAcmHandle = s_cdcAcmConfigList.config->classHandle;
    }

  /* Create stream buffer for ISR->read data flow. */
  s_recvStreamBuffer = xStreamBufferCreateStatic(sizeof(s_recvStreamArray),
    RECV_STREAM_BUFFER_TRIGGER_LEVEL, s_recvStreamArray, &s_recvStreamStruct );
  if (s_recvStreamBuffer == NULL) {
    //LOGE(TAG, "s_recvStreamBuffer creation failed!");
    return;
  }

  xSemaphore = xSemaphoreCreateMutexStatic( &xMutexBuffer );

  USB_DeviceIsrEnable();

  /* USB-RTC Contention Issue.
   *
   * There are currently 3 possible solutions:
   * Solution 1: Use a 12 second delay after the device powers on before the USB connection starts, delay occurs BEFORE scheduler start.
   *             Implemented in virtual_com.c, end of virtual_com_init_helper() function.
   * Solution 2: Use a 12 second delay after the device powers on before the USB connection starts, delay occurs AFTER scheduler start.
   *             Implemented in shell_recv.c, at the start of shell_recv_task() function.
   * Solution 3: In clock_config.c (variants/variant_ff2/clock_config.c), in function BOARD_BootClockPLL150M(),
   *             comment-out the line 'CLOCK_AttachClk(kXTAL32K_to_OSC32K);' - which is a partial solution that fixed USB but disables RTC
   *             unless additional code is added to switch to the 32kHZ FRO.
   *
   * The delay time of 12 seconds was arrived at experimentally.
   * <= 10000 ms =  causes USB 3.0 init error on David's Windows 10 computer
   * >= 11000 ms = WORKS
   * using 12000 for safety.
   *             */

#if defined(USBC_XTAL32K_COMPATBILITY_OPTION) && (USBC_XTAL32K_COMPATBILITY_OPTION == 1U)
  // USE THIS CODE FOR SOLUTION 1.
  #ifdef VARIANT_FF2
  SDK_DelayAtLeastUs(12000*1000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
  #endif
  virtual_com_delayed_start();
#elif defined(USBC_XTAL32K_COMPATBILITY_OPTION) && (USBC_XTAL32K_COMPATBILITY_OPTION == 2U)
  // do nothing
#elif defined(USBC_XTAL32K_COMPATBILITY_OPTION) && (USBC_XTAL32K_COMPATBILITY_OPTION == 3U)
  // USE THIS CODE FOR SOLUTION 3
  /* The following code has been moved to the separate function virtual_com_delayed_start().
   * This allows the virtual com to be initialized during a task, after other tasks have begun. */
  /*Add one delay here to make the DP pull down long enough to allow host to detect the previous disconnection.*/
  SDK_DelayAtLeastUs(5000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
  USB_DeviceRun(s_cdcVcom.deviceHandle);
#else
  #error "USBC_XTAL32K_COMPATBILITY_OPTION unrecognized"
#endif
}

void virtual_com_delayed_start(){
      /*Add one delay here to make the DP pull down long enough to allow host to detect the previous disconnection.*/
    SDK_DelayAtLeastUs(5000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    USB_DeviceRun(s_cdcVcom.deviceHandle);
}

///* Write data to virtual serial port. */
//ssize_t
//virtual_com_write_direct(char* buf, size_t count)
//{
//    usb_status_t error = kStatus_USB_Success;
//    if (buf==NULL || count == 0)
//    {
//         error = kStatus_USB_InvalidParameter;
//    }
//    else
//    {
//         if ((1 != s_cdcVcom.attach) || (1 != s_cdcVcom.startTransactions))
//         {
//              error=kStatus_USB_ControllerNotFound;
//         }
//         else
//         {
//#if 1
//           /* Use this code to send arbitrary large buffers*/
//             size_t len_remaining = count;
//             size_t buf_offset = 0;
//
//             while(len_remaining > 0){
//
//                 // Note: We never send exactly DATA_BUFF_SIZE as this causes the USB to hang.
//                 //       Instead, we send one less than DATA_BUFF_SIZE.
//                 size_t len_to_send = len_remaining < DATA_BUFF_SIZE ? len_remaining : DATA_BUFF_SIZE-1;
//                 memcpy(s_currSendBuf, buf+buf_offset, len_to_send);
//
//                 char buf[40];
//                 snprintf(buf,sizeof(buf),"%d [", count);
//                 debug_uart_puts( buf );
//
//                 g_sendFinished = false;
//                 if (USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_currSendBuf, len_to_send) != kStatus_USB_Success)
//                 {
//                      /* Failure to send Data Handling code here */
//                   error = kStatus_USB_Error;
//                   break;
//                 } else {
//                      /* Wait until transmission are done */
//                      while (!g_sendFinished && s_cdcVcom.startTransactions==1 && s_cdcVcom.attach==1) {
//                        snprintf(buf,sizeof(buf), "finished: %d start: %d attach: %d", g_sendFinished, s_cdcVcom.startTransactions, s_cdcVcom.attach);
//                        debug_uart_puts( buf );
//                        /*taskYIELD();*/
//                        };
//                      g_sendFinished = false;
//                      debug_uart_puts( "]" );
//
//                      len_remaining -= len_to_send;
//                      buf_offset += len_to_send;
//                 }
//             }
//#else
//             /* Use this code to send buffers less than DATA_BUFF_SIZE in size*/
//             memcpy(s_currSendBuf, buf, count > DATA_BUFF_SIZE ? DATA_BUFF_SIZE : count);
//             if (USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_currSendBuf, count > DATA_BUFF_SIZE ? DATA_BUFF_SIZE : count) != kStatus_USB_Success)
//             {
//                  /* Failure to send Data Handling code here */
//             } else {
//                  /* Wait until transmission are done */
//                  while (!g_sendFinished) {};
//                  g_sendFinished = false;
//             }
//
//#endif
//
//         }
//    }
//    return (error == kStatus_USB_Success) ? count : -1;
//}

/* Write data to virtual serial port. */
// buf - a pointer to s_currSendBuf
// count - the number of bytes to send from buf
static usb_status_t
virtual_com_write_direct(uint8_t* buf, size_t count)
{
    usb_status_t error = kStatus_USB_Success;
    if (buf==NULL || count == 0 || count >= DATA_BUFF_SIZE )
    {
         error = kStatus_USB_InvalidParameter;
    }
    else
    {
         if ((1 != s_cdcVcom.attach) || (1 != s_cdcVcom.startTransactions))
         {
              error = kStatus_USB_ControllerNotFound;
         }
         else
         {
//               char cbuf[40];
//               snprintf(cbuf,sizeof(cbuf),"%d [", count);
//               debug_uart_puts( cbuf );

               g_sendFinished = false;
               // (class_handle_t handle, uint8_t ep, uint8_t *buffer, uint32_t length)
               if (USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, buf, count) != kStatus_USB_Success)
               {
//                 debug_uart_puts("ERROR");
                    /* Failure to send Data Handling code here */
                 error = kStatus_USB_Error;
               } else {
                    /* Wait until transmission are done */
                    while (!g_sendFinished && s_cdcVcom.startTransactions==1 && s_cdcVcom.attach==1) {
//                      snprintf(cbuf,sizeof(cbuf), "finished: %d start: %d attach: %d", g_sendFinished, s_cdcVcom.startTransactions, s_cdcVcom.attach);
//                      debug_uart_puts( cbuf );
                      };
                    g_sendFinished = false;
//                    debug_uart_puts( "]" );
               }
           }
    }
    return error;
}

static inline size_t min(size_t a, size_t b){
  return a<b ? a : b;
}

/*
 * This is an experimental conditional that makes calls to virtual_com_write aggregate up to (DATA_BUFF_SIZE-1)
 * number of bytes before sending a packet over USB.
 * The size DATA_BUFF_SIZE-1 is used because it prevents a currently unhandled condition of sending exactly DATA_BUFF_SIZE bytes.
 */
#define VIRTUAL_COM_AGGREGATE_WRITES (0U)

/*
 * We use 1 less than the DATA_BUFF_SIZE, because when we use the full DATA_BUFF_SIZE, the USB hangs.
 */
#define MAX_SEND_SIZE (DATA_BUFF_SIZE-1)
static size_t s_currSendBuf_offset = 0;

/* Write data to virtual serial port. */
ssize_t
virtual_com_write(char* buf, size_t size)
{
  size_t buf_offset = 0;

  /*
   * We don't wait to take the semaphore. In the case virtual_com_write_direct is hung,
   * we want the other tasks to continue running.
   *
   */
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdPASS){ // portMAX_DELAY

    while ( (size - buf_offset) > 0 ){  //len_remaining <= (MAX_SEND_SIZE - s_currSendBuf_count
      size_t copy_len = min( MAX_SEND_SIZE - s_currSendBuf_offset, size - buf_offset);
      memcpy( &(s_currSendBuf[s_currSendBuf_offset]), &(buf[buf_offset]), copy_len );
      buf_offset += copy_len;
      s_currSendBuf_offset += copy_len;

#if (defined(VIRTUAL_COM_AGGREGATE_WRITES) && (VIRTUAL_COM_AGGREGATE_WRITES > 0U))
      if ( s_currSendBuf_offset == MAX_SEND_SIZE ){
#endif
        usb_status_t write_status = virtual_com_write_direct(s_currSendBuf, s_currSendBuf_offset);
        if(write_status == kStatus_USB_Success){
          s_currSendBuf_offset = 0;
        }else{
          s_currSendBuf_offset -= copy_len;
          buf_offset -= copy_len;
          break;
        }
      }
#if (defined(VIRTUAL_COM_AGGREGATE_WRITES) && (VIRTUAL_COM_AGGREGATE_WRITES > 0U))
    }
#endif

    xSemaphoreGive(xSemaphore);
  }
  return buf_offset; // TODO: update this value
}

// TODO: call flush somewhere.
void
virtual_com_flush(){
#if (defined(VIRTUAL_COM_AGGREGATE_WRITES) && (VIRTUAL_COM_AGGREGATE_WRITES > 0U))
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdPASS){
    virtual_com_write_direct(s_currSendBuf, s_currSendBuf_offset);
    s_currSendBuf_offset = 0;
    xSemaphoreGive(xSemaphore);
  }
#endif
}

/* Initialize virtual serial port. */
void virtual_com_init(void)
{
    NVIC_ClearPendingIRQ(USB0_IRQn);
    NVIC_ClearPendingIRQ(USB0_NEEDCLK_IRQn);
    NVIC_ClearPendingIRQ(USB1_IRQn);
    NVIC_ClearPendingIRQ(USB1_NEEDCLK_IRQn);

    POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY); /*< Turn on USB0 Phy */
    POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY); /*< Turn on USB1 Phy */

    /* reset the IP to make sure it's in reset state. */
    RESET_PeripheralReset(kUSB0D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HSL_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HMR_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1H_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1RAM_RST_SHIFT_RSTn);

#if defined(VARIANT_FF1) || defined(VARIANT_FF2) || defined(VARIANT_FF3)
    // FF1 board uses USB1 HS (High Speed) interface
    /* Enable usb1 host clock */
    CLOCK_EnableClock(kCLOCK_Usbh1);
    /* Put PHY powerdown under software control */
    *((uint32_t *)(USBHSH_BASE + 0x50)) = USBHSH_PORTMODE_SW_PDCOM_MASK;
      /* According to reference mannual, device mode setting has to be set by access usb host register */
    *((uint32_t *)(USBHSH_BASE + 0x50)) |= USBHSH_PORTMODE_DEV_ENABLE_MASK;
    /* Disable usb1 host clock */
    CLOCK_DisableClock(kCLOCK_Usbh1);
  
#elif defined(VARIANT_NFF1)
    // NFF1 board uses USB0 FS (Full Speed) interface

    /* Turn on USB0 Phy (not sure why we do this twice, but it was in example). */
    POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY); /*< Turn on USB Phy */
    /* Enable usb0 host clock */
    CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1, false);
    CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);
    CLOCK_EnableClock(kCLOCK_Usbhsl0);
    /*According to reference manual, device mode setting has to be set by access usb host register */
    *((uint32_t *)(USBFSH_BASE + 0x5C)) |= USBFSH_PORTMODE_DEV_ENABLE_MASK;
    /* Disable usb0 host clock */
    CLOCK_DisableClock(kCLOCK_Usbhsl0);
#else
#error "Unknown Morpheus variant--no USB initialization!"
#endif
	
	virtual_com_init_helper();
}


/* Read data from virtual serial port. */
ssize_t
virtual_com_read(char* buf, size_t len)
{
  USB_DeviceCdcAcmRecv(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, s_currRecvBuf, g_UsbDeviceCdcVcomDicEndpoints[1].maxPacketSize);
  return xStreamBufferReceive(s_recvStreamBuffer, buf, len, portMAX_DELAY);
}

/* Returns true if the virtual com port is attached (i.e. USB cable is plugged in) */
bool virtual_com_attached(){
  return s_cdcVcom.attach;
}

