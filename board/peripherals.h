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
#include "fsl_sctimer.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "fsl_flexspi.h"
#include "fsl_flexspi_dma.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/
/* Definitions for BOARD_InitPeripherals functional group */
/* Used DMA device. */
#define DMA0_DMA_BASEADDR DMA0
/* DMA0 interrupt vector ID (number). */
#define DMA0_IRQN DMA0_IRQn
/* DMA0 interrupt vector priority. */
#define DMA0_IRQ_PRIORITY 2
/* DMA0 interrupt handler identifier. */
#define DMA0_DriverIRQHandler DMA0_IRQHandler
/* Used DMA device. */
#define DMA1_DMA_BASEADDR DMA1
/* DMA1 interrupt vector ID (number). */
#define DMA1_IRQN DMA1_IRQn
/* DMA1 interrupt vector priority. */
#define DMA1_IRQ_PRIORITY 0
/* DMA1 interrupt handler identifier. */
#define DMA1_IRQHANDLER DMA1_IRQHandler
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
/* FC3_SENSOR_I2C interrupt vector priority. */
#define FC3_SENSOR_I2C_FLEXCOMM_IRQ_PRIORITY 5
/* Definition of peripheral ID */
#define FC5_DEBUG_UART_PERIPHERAL ((USART_Type *)FLEXCOMM5)
/* Definition of the clock source frequency */
#define FC5_DEBUG_UART_CLOCK_SOURCE 16000000UL
/* Definition of the backround buffer size */
#define FC5_DEBUG_UART_BACKGROUND_BUFFER_SIZE 256
/* FC5_DEBUG_UART interrupt vector ID (number). */
#define FC5_DEBUG_UART_IRQN FLEXCOMM5_IRQn
/* FC5_DEBUG_UART interrupt vector priority. */
#define FC5_DEBUG_UART_IRQ_PRIORITY 5
/* BOARD_InitPeripherals defines for PINT */
/* Definition of peripheral ID */
#define PINT_PERIPHERAL ((PINT_Type *) PINT_BASE)
/* PINT interrupt vector ID (number). */
#define PINT_PINT_0_IRQN PIN_INT0_IRQn
/* PINT interrupt vector priority. */
#define PINT_PINT_0_IRQ_PRIORITY 3
/* Definition of PINT interrupt ID for interrupt 1  */
#define PINT_INT_1 kPINT_PinInt1
/* Definition of PINT interrupt ID for interrupt 3  */
#define PINT_INT_3 kPINT_PinInt3
/* Definition of PINT interrupt ID for interrupt 4  */
#define PINT_INT_4 kPINT_PinInt4
/* Definition of PINT interrupt ID for interrupt 5  */
#define PINT_INT_5 kPINT_PinInt5
/* Definition of PINT interrupt ID for interrupt 6  */
#define PINT_INT_6 kPINT_PinInt6
/* Definition of PINT interrupt ID for interrupt 0  */
#define PINT_INT_0 kPINT_PinInt0
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
/* BOARD_InitPeripherals defines for SCT0 */
/* Definition of peripheral ID */
#define SCT0_PERIPHERAL SCT0
/* Definition of clock source frequency */
#define SCT0_CLOCK_FREQ CLOCK_GetFreq(kCLOCK_BusClk)
/* SCT0 interrupt vector ID (number). */
#define SCT0_IRQN SCT0_IRQn
/* SCT0 interrupt vector priority. */
#define SCT0_IRQ_PRIORITY 2
/* SCT0 interrupt handler identifier. */
#define SCT0_IRQHANDLER SCT0_IRQHandler
/* BOARD_InitPeripherals defines for FLEXCOMM4 */
/* Definition of peripheral ID */
#define FC4_AUDIO_I2S_PERIPHERAL ((I2S_Type *)FLEXCOMM4)
/* Definition of the clock source frequency */
#define FC4_AUDIO_I2S_CLOCK_SOURCE 2847667UL
/* Selected DMA channel number. */
#define FC4_AUDIO_I2S_TX_DMA_CHANNEL 9
/* Used DMA device. */
#define FC4_AUDIO_I2S_TX_DMA_BASEADDR DMA0
/* Definition of peripheral ID */
#define NAND_FLEXSPI_PERIPHERAL FLEXSPI
/* Transfer buffer size. */
#define NAND_FLEXSPI_TRANSFER_BUFFER_SIZE_0 1
/* Selected DMA channel number. */
#define NAND_FLEXSPI_TX_DMA_CHANNEL 29
/* Used DMA device. */
#define NAND_FLEXSPI_TX_DMA_BASEADDR DMA1
/* Selected DMA channel number. */
#define NAND_FLEXSPI_RX_DMA_CHANNEL 28
/* Used DMA device. */
#define NAND_FLEXSPI_RX_DMA_BASEADDR DMA1

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
extern const sctimer_config_t SCT0_initConfig;
extern const sctimer_pwm_signal_param_t SCT0_pwmSignalsConfig[3];
extern uint32_t SCT0_pwmEvent[3];
extern const i2s_config_t FC4_AUDIO_I2S_config;
extern dma_handle_t FC4_AUDIO_I2S_TX_Handle;
extern i2s_dma_handle_t FC4_AUDIO_I2S_Tx_DMA_Handle;
extern const flexspi_config_t NAND_FLEXSPI_config;
extern flexspi_device_config_t NAND_FLEXSPI_config_NAND;
extern const flexspi_transfer_t NAND_FLEXSPI_config_transfer_NAND;
extern dma_handle_t NAND_FLEXSPI_RX_Handle;
extern dma_handle_t NAND_FLEXSPI_TX_Handle;
extern flexspi_dma_handle_t NAND_FLEXSPI_DMA_Handle;

/***********************************************************************************************************************
 * Callback functions
 **********************************************************************************************************************/
/* INT_1 callback function for the PINT component */
extern void charger_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_3 callback function for the PINT component */
extern void hrm_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_4 callback function for the PINT component */
extern void accel_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_5 callback function for the PINT component */
extern void user_button1_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_6 callback function for the PINT component */
extern void user_button2_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* INT_0 callback function for the PINT component */
extern void eeg_drdy_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status);
/* SPI DMA callback function for the FC1_EEG_SPI component (init. function BOARD_InitPeripherals)*/
extern void eeg_dma_rx_complete_isr(SPI_Type *,spi_dma_handle_t *,status_t ,void *);
/* I2S DMA callback function for the FC4_AUDIO_I2S component (init. function BOARD_InitPeripherals)*/
extern void audio_i2s_isr(I2S_Type *,i2s_dma_handle_t *,status_t ,void *);
/* FLEXSPI DMA callback function for the NAND_FLEXSPI component (init. function BOARD_InitPeripherals)*/
extern void nand_flexspi_isr(FLEXSPI_Type *, flexspi_dma_handle_t *, status_t, void *);

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
