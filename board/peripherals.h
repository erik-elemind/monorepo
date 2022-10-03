/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

/***********************************************************************************************************************
 * Included files
 **********************************************************************************************************************/
#include "fsl_dma.h"
#include "fsl_common.h"
#include "fsl_i2c.h"
#include "fsl_i2c_freertos.h"
#include "fsl_usart.h"
#include "fsl_usart_freertos.h"
#include "fsl_clock.h"
#include "fsl_pint.h"
#include "fsl_spi.h"
#include "fsl_spi_dma.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/
/* Definitions for BOARD_InitPeripherals functional group */
/* Used DMA device. */
#define DMA0_DMA_BASEADDR DMA0
/* BOARD_InitPeripherals defines for FLEXCOMM2 */
/* Definition of peripheral ID */
#define FC2_BATT_I2C_PERIPHERAL ((I2C_Type *)FLEXCOMM2)
/* Definition of the clock source frequency */
#define FC2_BATT_I2C_CLOCK_SOURCE 16000000UL
/* FC2_BATT_I2C interrupt vector ID (number). */
#define FC2_BATT_I2C_FLEXCOMM_IRQN FLEXCOMM2_IRQn
/* FC2_BATT_I2C interrupt vector priority. */
#define FC2_BATT_I2C_FLEXCOMM_IRQ_PRIORITY 5
/* BOARD_InitPeripherals defines for FLEXCOMM3 */
/* Definition of peripheral ID */
#define FC3_SENSOR_I2C_PERIPHERAL ((I2C_Type *)FLEXCOMM3)
/* Definition of the clock source frequency */
#define FC3_SENSOR_I2C_CLOCK_SOURCE 16000000UL
/* FC3_SENSOR_I2C interrupt vector ID (number). */
#define FC3_SENSOR_I2C_FLEXCOMM_IRQN FLEXCOMM3_IRQn
/* Definition of peripheral ID */
#define FC5_DEBUG_UART_PERIPHERAL ((USART_Type *)FLEXCOMM5)
/* Definition of the clock source frequency */
#define FC5_DEBUG_UART_CLOCK_SOURCE 16000000UL
/* Definition of the backround buffer size */
#define FC5_DEBUG_UART_BACKGROUND_BUFFER_SIZE 256
/* FC5_DEBUG_UART interrupt vector ID (number). */
#define FC5_DEBUG_UART_IRQN FLEXCOMM5_IRQn
/* BOARD_InitPeripherals defines for PINT */
/* Definition of peripheral ID */
#define PINT_PERIPHERAL ((PINT_Type *) PINT_BASE)
/* Definition of PINT interrupt ID for interrupt 1  */
#define PINT_INT_1 kPINT_PinInt1
/* Definition of PINT interrupt ID for interrupt 2  */
#define PINT_INT_2 kPINT_PinInt2
/* Definition of PINT interrupt ID for interrupt 3  */
#define PINT_INT_3 kPINT_PinInt3
/* Definition of PINT interrupt ID for interrupt 4  */
#define PINT_INT_4 kPINT_PinInt4
/* Definition of PINT interrupt ID for interrupt 5  */
#define PINT_INT_5 kPINT_PinInt5
/* Definition of PINT interrupt ID for interrupt 6  */
#define PINT_INT_6 kPINT_PinInt6
/* Definition of PINT interrupt ID for interrupt 7  */
#define PINT_INT_7 kPINT_PinInt7
/* Definition of peripheral ID */
#define FC0_BLE_UART_PERIPHERAL ((USART_Type *)FLEXCOMM0)
/* Definition of the clock source frequency */
#define FC0_BLE_UART_CLOCK_SOURCE 16000000UL
/* Definition of the backround buffer size */
#define FC0_BLE_UART_BACKGROUND_BUFFER_SIZE 2048
/* FC0_BLE_UART interrupt vector ID (number). */
#define FC0_BLE_UART_IRQN FLEXCOMM0_IRQn
/* FC0_BLE_UART interrupt vector priority. */
#define FC0_BLE_UART_IRQ_PRIORITY 5
/* BOARD_InitPeripherals defines for FLEXCOMM1 */
/* Definition of peripheral ID */
#define FC1_EEG_SPI_PERIPHERAL ((SPI_Type *)FLEXCOMM1)
/* Definition of the clock source frequency */
#define FC1_EEG_SPI_CLOCK_SOURCE 16000000UL
/* Selected DMA channel number. */
#define FC1_EEG_SPI_RX_DMA_CHANNEL 2
/* Used DMA device. */
#define FC1_EEG_SPI_RX_DMA_BASEADDR DMA0
/* Selected DMA channel number. */
#define FC1_EEG_SPI_TX_DMA_CHANNEL 3
/* Used DMA device. */
#define FC1_EEG_SPI_TX_DMA_BASEADDR DMA0

/***********************************************************************************************************************
 * Global variables
 **********************************************************************************************************************/
extern i2c_rtos_handle_t FC2_BATT_I2C_rtosHandle;
extern const i2c_master_config_t FC2_BATT_I2C_config;
extern i2c_rtos_handle_t FC3_SENSOR_I2C_rtosHandle;
extern const i2c_master_config_t FC3_SENSOR_I2C_config;
extern usart_rtos_handle_t FC5_DEBUG_UART_rtos_handle;
extern usart_handle_t FC5_DEBUG_UART_usart_handle;
extern struct rtos_usart_config FC5_DEBUG_UART_rtos_config;
extern usart_rtos_handle_t FC0_BLE_UART_rtos_handle;
extern usart_handle_t FC0_BLE_UART_usart_handle;
extern struct rtos_usart_config FC0_BLE_UART_rtos_config;
extern const spi_master_config_t FC1_EEG_SPI_config;
extern dma_handle_t FC1_EEG_SPI_RX_Handle;
extern dma_handle_t FC1_EEG_SPI_TX_Handle;
extern spi_dma_handle_t FC1_EEG_SPI_DMA_Handle;

/***********************************************************************************************************************
 * Callback functions
 **********************************************************************************************************************/
/* INT_1 callback function for the PINT component */
extern void charger_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_2 callback function for the PINT component */
extern void pb_int_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_3 callback function for the PINT component */
extern void hrm_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_4 callback function for the PINT component */
extern void accel_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_5 callback function for the PINT component */
extern void user_button1_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_6 callback function for the PINT component */
extern void user_button2_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_7 callback function for the PINT component */
extern void power_button_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* SPI DMA callback function for the FC1_EEG_SPI component (init. function BOARD_InitPeripherals)*/
extern void eeg_dma_rx_complete_isr(SPI_Type *,spi_dma_handle_t *,status_t ,void *);

/***********************************************************************************************************************
 * Initialization functions
 **********************************************************************************************************************/

void BOARD_InitPeripherals(void);

/***********************************************************************************************************************
 * BOARD_InitBootPeripherals function
 **********************************************************************************************************************/
void BOARD_InitBootPeripherals(void);

#if defined(__cplusplus)
}
#endif

#endif /* _PERIPHERALS_H_ */
