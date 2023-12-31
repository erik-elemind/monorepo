#ifndef BOARD_FF3_H
#define BOARD_FF3_H

// For HAL Driver Handles:
#include "fsl_i2c.h"
#include "fsl_i2c_freertos.h"
#include "fsl_usart_freertos.h"

#include "fsl_usart.h"


#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_reset.h"
#include "fsl_gpio.h"

#include "fsl_spi_dma.h"

//
// Board-specific Configuration Values (for build time)
//
#include "clock_config.h"
#include "peripherals.h"
#include "pin_mux.h"
//#include "variants/common/RTE_Device.h"
//#include "variants/common/tzm_config.h"

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
#define USART_BLE_CLK_FREQ       16000000U
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
#define USART_DEBUG_RTOS_HANDLE    FC5_DEBUG_UART_rtos_handle  // Declared and init'd in peripherals.c
#define USART_DEBUG_RST            kFC5_RST_SHIFT_RSTn    // Passed to RESET_ClearPeripheralReset()
#define USART_DEBUG_NVIC_PRIORITY  3  // Interrupt priority: 1 is highest priority
#define USART_DEBUG_IRQn           FLEXCOMM5_IRQn
#define USART_DEBUG_BUFFER_SIZE    FC5_BACKGROUND_BUFFER_SIZE

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

#define SPI_EEG_BASE          FC1_EEG_SPI_PERIPHERAL // Declared and init'd in peripherals.c
#define SPI_EEG_DMA_HANDLE    FC1_EEG_SPI_DMA_Handle // Declared and init'd in peripherals.c
extern spi_dma_handle_t SPI_EEG_DMA_HANDLE;
void BOARD_InitEEGSPI();



#define SPI_FLASH_BASE         0 // ToDo: Port Over
#define SPI_FLASH_DMA_HANDLE   0 // ToDo: Port Over
#define SPI_DMA_TX_HANDLE      0
#define SPI_DMA_RX_HANDLE      0
void BOARD_InitFlashSPI();


/******************************************************************************
 * I2C on FLEXCOMM 4F
 * Used By:
 *   Heart Rate Monitor
 *   Ambient Light
 *   On Head Test Connector
 *   Accelerometer
 *   Temperature
 *   Audio Amplifier
 *****************************************************************************/

// This define redirects to a define from peripheral.h/c
#define SENSOR_I2C_RTOS_HANDLE     FC3_SENSOR_I2C_rtosHandle
#define ACCEL_I2C_RTOS_HANDLE      FC3_SENSOR_I2C_rtosHandle
#define AUDIO_I2C_RTOS_HANDLE      FC3_SENSOR_I2C_rtosHandle
#define ALS_I2C_RTOS_HANDLE        FC3_SENSOR_I2C_rtosHandle
#define HRM_I2C_RTOS_HANDLE        FC3_SENSOR_I2C_rtosHandle

/******************************************************************************
 * I2C on FLEXCOMM 5
 * Used By:
 *   Battery Charger
 *****************************************************************************/

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

#define EEG_PWDN_GPIO  BOARD_INITPINS_EEG_PWDn_GPIO
#define EEG_PWDN_PORT  BOARD_INITPINS_EEG_PWDn_PORT
#define EEG_PWDN_PIN   BOARD_INITPINS_EEG_PWDn_PIN

#define EEG_LDO_EN_GPIO BOARD_INITPINS_EEG_LDO_EN_GPIO
#define EEG_LDO_EN_PORT BOARD_INITPINS_EEG_LDO_EN_PORT
#define EEG_LDO_EN_PIN BOARD_INITPINS_EEG_LDO_EN_PIN

/*
 * Set whether the processor should control the
 * EEG LDO voltage regulator through the enable pin.
 */
#define EEG_LDO_CONTROL      1
/*
 * Set whether the processor should control the
 * EEG negative charge pump through the enable pin.
 */
#define EEG_CP_CONTROL      0

/*
 * Set the logic-level used to enable the
 * EEG LDO voltage regulator through the enable pin.
 */
#define EEG_LDO_ENABLE_LEVEL 1
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

#define AUDIO_I2S_BASE          FC4_AUDIO_I2S_PERIPHERAL
#define AUDIO_I2S_TX_HANDLE     FC4_AUDIO_I2S_TX_Handle
#define AUDIO_I2S_DMA_TX_HANDLE FC4_AUDIO_I2S_Tx_DMA_Handle

/******************************************************************************
 * Battery Charger
 *****************************************************************************/

#define BATTERY_CHARGER_ENABLE_PORT 0 //ToDo: Remove for revD
#define BATTERY_CHARGER_ENABLE_PIN 0 //ToDo: Remove for revD

#define BATTERY_CHARGER_STATUS_PORT 0 //ToDo: Remove for revD
#define BATTERY_CHARGER_STATUS_PIN 0 //ToDo: Remove for revD

/******************************************************************************
 * RED/GREEN/BLUE LED PWM
 *****************************************************************************/

#define SCTIMER_CLK_FREQ       CLOCK_GetFreq(kCLOCK_BusClk)

#define LED_SCT_PERIPHERAL        SCT0_PERIPHERAL

// SCT0_initConfig, SCT0_pwmSignalsConfig, and SCT0_pwmEvent
// is defined in peripherals.h/c, which is auto-generated by MCUXpresso Config Tools.
#define LED_SCT_initConfig        SCT0_initConfig
#define LED_SCT_pwmSignalsConfig  SCT0_pwmSignalsConfig
#define LED_SCT_pwmEvent          SCT0_pwmEvent

// SCT0_pwmSignalsConfig and SCT0_pwmEvent are arrays
// which can be indexed into using the following #defines
#define LED_SCT_RED_INDEX          0
#define LED_SCT_GRN_INDEX          1
#define LED_SCT_BLU_INDEX          2

#define LED_PIO_PERIPHERAL        IOPCTL
// MCUXpresso Config Tools configures the GPIO pin separately from the SCTimer
// As of this writing, we do not have a good way to use the auto-generates
// files from the config tool to get this mapping.
#define LED_RED_PORT (0U)
#define LED_RED_PIN  (27U)
#define LED_GRN_PORT (0U)
#define LED_GRN_PIN  (26U)
#define LED_BLU_PORT (2U)
#define LED_BLU_PIN  (14U)

// duty_cycle values range from 1 to 100 precent.
void led_set_rgb(uint8_t red_duty_cycle_percent, uint8_t grn_duty_cycle_percent, uint8_t blu_duty_cycle_percent, bool tristate);

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
#define ACTIVITY_BUTTON_PORT     BOARD_INITPINS_ACTIVITY_BUTTON_PORT
#define ACTIVITY_BUTTON_PIN      BOARD_INITPINS_ACTIVITY_BUTTON_PIN
#define ACTIVITY_BUTTON_UP_LEVEL 0

#define VOL_UP_BUTTON_PORT     BOARD_INITPINS_USER_BUTTON1_PORT
#define VOL_UP_BUTTON_PIN      BOARD_INITPINS_USER_BUTTON1_PIN
#define VOL_UP_BUTTON_UP_LEVEL 1

#define VOL_DOWN_BUTTON_PORT     BOARD_INITPINS_USER_BUTTON2_PORT
#define VOL_DOWN_BUTTON_PIN      BOARD_INITPINS_USER_BUTTON2_PIN
#define VOL_DOWN_BUTTON_UP_LEVEL 0

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
