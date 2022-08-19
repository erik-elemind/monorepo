#ifndef BOARD_FF3_H
#define BOARD_FF3_H

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
#define USART_BLE_RTOS_HANDLE    FC0_BLE_UART_rtos_handle // Declared and init'd in peripherals.c
#define USART_BLE_RST            kFC0_RST_SHIFT_RSTn    // Passed to RESET_ClearPeripheralReset()
#define USART_BLE_INSTANCE       0U
#define USART_BLE_IRQn           FLEXCOMM0_IRQn
#define USART_BLE_BASEADDR       USART0
#define USART_BLE_CLK_FREQ       12000000U
#define USART_BLE_BAUDRATE       115200
#define USART_BLE_PARITY         kUSART_ParityDisabled
#define USART_BLE_STOPBITS       kUSART_OneStopBit

#define USART_BLE_TYPE           kSerialPort_Uart
#define USART_BLE_NVIC_PRIORITY  3  // Interrupt priority: 1 is highest priority

#define BLE_RESETN_PORT     BOARD_INITPINS_BLE_RESETn_PORT
#define BLE_RESETN_PIN      BOARD_INITPINS_BLE_RESETn_PIN

extern usart_rtos_handle_t       USART_BLE_RTOS_HANDLE;
int BOARD_InitBLE();

/******************************************************************************
 * DEBUG UART
 *****************************************************************************/
#define USART_DEBUG_RTOS_HANDLE    FC1_DEBUG_UART_rtos_handle  // Declared and init'd in peripherals.c
#define USART_DEBUG_RST            kFC1_RST_SHIFT_RSTn    // Passed to RESET_ClearPeripheralReset()
#define USART_DEBUG_NVIC_PRIORITY  3  // Interrupt priority: 1 is highest priority
#define USART_DEBUG_IRQn           FLEXCOMM1_IRQn
#define USART_DEBUG_BUFFER_SIZE    FLEXCOMM1_BACKGROUND_BUFFER_SIZE

extern usart_rtos_handle_t         USART_DEBUG_RTOS_HANDLE;
int BOARD_InitDebugConsole();

/******************************************************************************
 * SPI busses
 *****************************************************************************/
#if 0  // Now using SPI DMA instead of SPI RTOS. TODO: Remove this.
  #define SPI_EEG_RTOS_HANDLE    FLEXCOMM2_rtosHandle // Declared and init'd in peripherals.c
  #define SPI_EEG_RTOS_IRQn      FLEXCOMM2_IRQn
  extern spi_rtos_handle_t         SPI_EEG_RTOS_HANDLE;
#endif

#define SPI_EEG_BASE          FC2_EEG_SPI_PERIPHERAL // Declared and init'd in peripherals.c
#define SPI_EEG_DMA_HANDLE    FC2_EEG_SPI_DMA_Handle // Declared and init'd in peripherals.c
extern spi_dma_handle_t SPI_EEG_DMA_HANDLE;
void BOARD_InitEEGSPI();



#define SPI_FLASH_BASE         FC8_FLASH_SPI_PERIPHERAL // Declared and init'd in peripherals.c
#define SPI_FLASH_DMA_HANDLE   FC8_FLASH_SPI_DMA_Handle // Declared and init'd in peripherals.c
#define SPI_DMA_TX_HANDLE      FC8_FLASH_SPI_RX_Handle
#define SPI_DMA_RX_HANDLE      FC8_FLASH_SPI_TX_Handle
void BOARD_InitFlashSPI();



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
#define I2C4_RTOS_HANDLE      FC6_I2C_rtosHandle

/******************************************************************************
 * I2C on FLEXCOMM 5
 * Used By:
 *   Battery Charger
 *****************************************************************************/

// This define redirects to a define from peripheral.h/c
#define I2C5_RTOS_HANDLE      FC5_BATTERY_I2C_rtosHandle

/******************************************************************************
 * EEG
 *****************************************************************************/


#define EEG_DRDY_GPIO  BOARD_INITPINS_EEG_DRDYn_GPIO
#define EEG_DRDY_PORT  BOARD_INITPINS_EEG_DRDYn_PORT
#define EEG_DRDY_PIN   BOARD_INITPINS_EEG_DRDYn_PIN

#define EEG_START_GPIO BOARD_INITPINS_EEG_START_GPIO
#define EEG_START_PORT BOARD_INITPINS_EEG_START_PORT
#define EEG_START_PIN  BOARD_INITPINS_EEG_START_PIN

#define EEG_RESET_GPIO BOARD_INITPINS_EEG_RESETn_GPIO
#define EEG_RESET_PORT BOARD_INITPINS_EEG_RESETn_PORT
#define EEG_RESET_PIN  BOARD_INITPINS_EEG_RESETn_PIN

#define EEG_PWDN_GPIO  BOARD_INITPINS_EEG_PWDNn_GPIO
#define EEG_PWDN_PORT  BOARD_INITPINS_EEG_PWDNn_PORT
#define EEG_PWDN_PIN   BOARD_INITPINS_EEG_PWDNn_PIN

/*
 * Set whether the processor should control the
 * EEG LDO voltage regulator through the enable pin.
 */
#define EEG_LDO_CONTROL      1
/*
 * Set whether the processor should control the
 * EEG negative charge pump through the enable pin.
 */
#define EEG_CP_CONTROL      1

/*
 * Set the logic-level used to enable the
 * EEG LDO voltage regulator through the enable pin.
 */
#define EEG_LDO_ENABLE_LEVEL 0
/*
 * Set the logic-level used to enable the
 * EEG negative charge pump through the enable pin.
 */
#define EEG_CP_ENABLE_LEVEL 1

/******************************************************************************
 * EEG PMIC
 *****************************************************************************/

/******************************************************************************
 * Sound Pressure Level, Microphone
 *****************************************************************************/

/******************************************************************************
 * Heart Rate Monitor (HRM)
 *****************************************************************************/

#define HRM_RED_ENABLE_PORT BOARD_INITPINS_HRM_RED_ANODE_EN_PORT
#define HRM_RED_ENABLE_PIN BOARD_INITPINS_HRM_RED_ANODE_EN_PIN
#define HRM_IR_ENABLE_PORT BOARD_INITPINS_HRM_IR_ANODE_EN_PORT
#define HRM_IR_ENABLE_PIN BOARD_INITPINS_HRM_IR_ANODE_EN_PIN
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

#define AUDIO_I2S_BASE          FC3_AUDIO_I2S_PERIPHERAL
#define AUDIO_I2S_TX_HANDLE     FC3_AUDIO_I2S_TX_Handle
#define AUDIO_I2S_DMA_TX_HANDLE FC3_AUDIO_I2S_Tx_DMA_Handle

/******************************************************************************
 * Battery Charger
 *****************************************************************************/

#define BATTERY_CHARGER_ENABLE_PORT BOARD_INITPINS_BAT_CEn_PORT
#define BATTERY_CHARGER_ENABLE_PIN BOARD_INITPINS_BAT_CEn_PIN

#define BATTERY_CHARGER_STATUS_PORT BOARD_INITPINS_BAT_STATn_PORT
#define BATTERY_CHARGER_STATUS_PIN BOARD_INITPINS_BAT_STATn_PIN

/******************************************************************************
 * RED/GREEN/BLUE LED PWM
 *****************************************************************************/

#define SCTIMER_CLK_FREQ       CLOCK_GetFreq(kCLOCK_BusClk)
// SCT0_pwmSignalsConfig is defined in peripherals.h/c,
// and is auto-generated by MCUXpresso Config Tools.
#define SCTIMER_OUT_RED_LED    SCT0_LED_pwmSignalsConfig[2].output
#define SCTIMER_OUT_GRN_LED    SCT0_LED_pwmSignalsConfig[1].output
#define SCTIMER_OUT_BLU_LED    SCT0_LED_pwmSignalsConfig[0].output
#define RED_LED_TRUE_PWM_LEVEL SCT0_LED_pwmSignalsConfig[2].level
#define GRN_LED_TRUE_PWM_LEVEL SCT0_LED_pwmSignalsConfig[1].level
#define BLU_LED_TRUE_PWM_LEVEL SCT0_LED_pwmSignalsConfig[0].level
#define RED_LED_EVENT          2
#define GRN_LED_EVENT          1
#define BLU_LED_EVENT          0

// duty_cycle values range from 1 to 100 precent.
void BOARD_SetRGB(uint8_t red_duty_cycle_percent, uint8_t grn_duty_cycle_percent, uint8_t blu_duty_cycle_percent);

/******************************************************************************
 * BUTTONS
 *****************************************************************************/

/*
 * The following button-related defines are used in button_config.h/c
 * The button defines must be sequential sequentially. That is:
 * If BUTTON_2_*** defines exist then BUTTON_1_*** defines must exist as well.
 * If BUTTON_3_*** defines exist then BUTTON_2_*** defines must exist as well.
 *
 * This define structure makes it easier to add BUTTON_3_*** defines in the future,
 * with minimal changes in button_config.c.
 */
#define LED_PERIPHERAL        SCT0_LED_PERIPHERAL
#define LED_pwmEvent          SCT0_LED_pwmEvent
#define LED_initConfig        SCT0_LED_initConfig

#define POWER_BUTTON_PORT     BOARD_INITPINS_POWER_BTNn_PORT
#define POWER_BUTTON_PIN      BOARD_INITPINS_POWER_BTNn_PIN
#define POWER_BUTTON_UP_LEVEL 1

#define VOL_UP_BUTTON_PORT     BOARD_INITPINS_USER_BUTTON1_PORT
#define VOL_UP_BUTTON_PIN      BOARD_INITPINS_USER_BUTTON1_PIN
#define VOL_UP_BUTTON_UP_LEVEL 1

#define VOL_DOWN_BUTTON_PORT     BOARD_INITPINS_USER_BUTTON2_PORT
#define VOL_DOWN_BUTTON_PIN      BOARD_INITPINS_USER_BUTTON2_PIN
#define VOL_DOWN_BUTTON_UP_LEVEL 1

/******************************************************************************
 * DEBUG LED
 *****************************************************************************/

#define DEBUG_LED_GPIO BOARD_INITPINS_DEBUG_LEDn_GPIO
#define DEBUG_LED_PORT BOARD_INITPINS_DEBUG_LEDn_PORT
#define DEBUG_LED_PIN BOARD_INITPINS_DEBUG_LEDn_PIN
#define DEBUG_LED_PIN_MASK BOARD_INITPINS_DEBUG_LEDn_PIN_MASK

/******************************************************************************
 * IR LED
 *****************************************************************************/

#define IR_LED_GPIO BOARD_INITPINS_IR_LED_GPIO
#define IR_LED_PORT BOARD_INITPINS_IR_LED_PORT
#define IR_LED_PIN BOARD_INITPINS_IR_LED_PIN
#define IR_LED_PIN_MASK BOARD_INITPINS_IR_LED_PIN_MASK

/******************************************************************************
 * COLOR LED
 *****************************************************************************/

#define COLOR_LED_GPIO BOARD_INITPINS_COLOR_LED_GPIO
#define COLOR_LED_PORT BOARD_INITPINS_COLOR_LED_PORT
#define COLOR_LED_PIN BOARD_INITPINS_COLOR_LED_PIN
#define COLOR_LED_PIN_MASK BOARD_INITPINS_COLOR_LED_PIN_MASK

/******************************************************************************
 * SQW
 *****************************************************************************/

#define SQW_GPIO BOARD_INITPINS_SQW_GPIO
#define SQW_PORT BOARD_INITPINS_SQW_PORT
#define SQW_PIN BOARD_INITPINS_SQW_PIN
#define SQW_PIN_MASK BOARD_INITPINS_SQW_PIN_MASK

/******************************************************************************
 * MICRO CLOCK
 *****************************************************************************/

#define MICRO_CLOCK_NVIC_PRIORITY 5

#ifdef __cplusplus
}
#endif


#endif  // BOARD_FF3_H
