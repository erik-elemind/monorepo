#ifndef BOARD_EVKS_H
#define BOARD_EVKS_H


// For HAL Driver Handles:
#include "fsl_i2c.h"
#include "fsl_i2c_freertos.h"

#include "fsl_usart.h"
#include "fsl_usart_freertos.h"

#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_reset.h"
#include "fsl_gpio.h"
#include "fsl_iocon.h"

//
// Board-specific Configuration Values (for build time)
//
#include "variants/common/board.h"
#include "variants/common/clock_config.h"
#include "variants/common/peripherals.h"
#include "variants/common/pin_mux.h"
#include "variants/common/RTE_Device.h"
#include "variants/common/tzm_config.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// MCUXpresso Drivers: #defines for bus names and Flexcomm IRQs
// Global externs named here are declared and init'd in main.cpp.
//

/******************************************************************************
 * BLUETOOTH
 *****************************************************************************/

// NOTE: IRQn FLEXCOMM0_IRQn, INSTANCE 0U, BASEADDR USART0, and RST kFC0_RST_SHIFT_RSTn
// must all use the same Flexcomm number (zero, one, two, etc.). Using zero here.
#define USART_BLE_RST            kFC0_RST_SHIFT_RSTn    // Passed to RESET_ClearPeripheralReset()
#define USART_BLE_INSTANCE       0U
#define USART_BLE_IRQn           FLEXCOMM0_IRQn
#define USART_BLE_BASEADDR       USART0
#define USART_BLE_CLK_FREQ       12000000U
#define USART_BLE_BAUDRATE       115200
#define USART_BLE_PARITY         kUSART_ParityDisabled
#define USART_BLE_STOPBITS       kUSART_OneStopBit

#define USART_BLE_TYPE           kSerialPort_Uart
#define USART_BLE_RTOS_HANDLE    g_usart_ble_rtos_handle
#define USART_BLE_NVIC_PRIORITY  3  // Interrupt priority: 1 is highest priority

extern usart_rtos_handle_t       USART_BLE_RTOS_HANDLE;
int BOARD_InitBLE();

/******************************************************************************
 * DEBUG UART
 *****************************************************************************/

#define USART_DEBUG_RST            kFC1_RST_SHIFT_RSTn    // Passed to RESET_ClearPeripheralReset()
#define USART_DEBUG_INSTANCE       1U
#define USART_DEBUG_IRQn           FLEXCOMM1_IRQn
#define USART_DEBUG_BASEADDR       USART1
#define USART_DEBUG_CLK_FREQ       12000000U
#define USART_DEBUG_BAUDRATE       115200
#define USART_DEBUG_PARITY         kUSART_ParityDisabled
#define USART_DEBUG_STOPBITS       kUSART_OneStopBit

#define USART_DEBUG_TYPE           kSerialPort_Uart
#define USART_DEBUG_RTOS_HANDLE    g_usart_debug_rtos_handle
#define USART_DEBUG_NVIC_PRIORITY  3  // Interrupt priority: 1 is highest priority

extern usart_rtos_handle_t         USART_DEBUG_RTOS_HANDLE;
int BOARD_InitDebugConsole();

/******************************************************************************
 * I2C on FLEXCOMM 4
 * Used By:
 *   Heart Rate Monitor
 *   Ambient Light
 *   On Head Test Connector
 *   Accelerometer
 *   Temperature
 *   Audio Amplifier
 *****************************************************************************/

// This define redirects to a define from peripheral.h/c
#define I2C4_RTOS_HANDLE      FLEXCOMM4_rtosHandle;

/******************************************************************************
 * I2C on FLEXCOMM 5
 * Used By:
 *   Battery Charger
 *****************************************************************************/

// This define redirects to a define from peripheral.h/c
#define I2C5_RTOS_HANDLE      FLEXCOMM5_rtosHandle;

/******************************************************************************
 * EEG
 *****************************************************************************/

/******************************************************************************
 * EEG PMIC
 *****************************************************************************/

/******************************************************************************
 * Sound Pressure Level, Microphone
 *****************************************************************************/

/******************************************************************************
 * Heart Rate Monitor (HRM)
 *****************************************************************************/

#define HRM_RED_ENABLE_PORT 0
#define HRM_RED_ENABLE_PIN 0
#define HRM_IR_ENABLE_PORT 0
#define HRM_IR_ENABLE_PIN 15
#define HRM_PINT_PIN kPINT_PinInt0

/******************************************************************************
 * Ambient Light Sensor (ALS)
 *****************************************************************************/

/******************************************************************************
 * Accelerometer
 *****************************************************************************/

/******************************************************************************
 * Temperature Sensor
 *****************************************************************************/

/******************************************************************************
 * Audio Amplifier
 *****************************************************************************/

/******************************************************************************
 * Battery Charger
 *****************************************************************************/

#ifdef __cplusplus
}
#endif


#endif  // BOARD_EVKS_H
