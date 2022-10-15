/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

/*!
 * @addtogroup pin_mux
 * @{
 */

/***********************************************************************************************************************
 * API
 **********************************************************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Calls initialization functions.
 *
 */
void BOARD_InitBootPins(void);

#define IOPCTL_PIO_ANAMUX_DI 0x00u        /*!<@brief Analog mux is disabled */
#define IOPCTL_PIO_ANAMUX_EN 0x0200u      /*!<@brief Analog mux is enabled */
#define IOPCTL_PIO_FULLDRIVE_DI 0x00u     /*!<@brief Normal drive */
#define IOPCTL_PIO_FUNC0 0x00u            /*!<@brief Selects pin function 0 */
#define IOPCTL_PIO_FUNC1 0x01u            /*!<@brief Selects pin function 1 */
#define IOPCTL_PIO_FUNC3 0x03u            /*!<@brief Selects pin function 3 */
#define IOPCTL_PIO_FUNC5 0x05u            /*!<@brief Selects pin function 5 */
#define IOPCTL_PIO_FUNC6 0x06u            /*!<@brief Selects pin function 6 */
#define IOPCTL_PIO_INBUF_DI 0x00u         /*!<@brief Disable input buffer function */
#define IOPCTL_PIO_INV_DI 0x00u           /*!<@brief Input function is not inverted */
#define IOPCTL_PIO_PSEDRAIN_DI 0x00u      /*!<@brief Pseudo Output Drain is disabled */
#define IOPCTL_PIO_PULLDOWN_EN 0x00u      /*!<@brief Enable pull-down function */
#define IOPCTL_PIO_PUPD_DI 0x00u          /*!<@brief Disable pull-up / pull-down function */
#define IOPCTL_PIO_SLEW_RATE_NORMAL 0x00u /*!<@brief Normal mode */

/*! @name FLEXSPI0A_SCLK (coord T9), NAND_SCK
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_NAND_SCK_PERIPHERAL FLEXSPI      /*!<@brief Peripheral name */
#define BOARD_INITPINS_NAND_SCK_SIGNAL FLEXSPI_A_SCLK   /*!<@brief Signal name */
                                                        /* @} */

/*! @name FLEXSPI0A_SS0_N (coord T4), NAND_CSn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_NAND_CSn_PERIPHERAL FLEXSPI       /*!<@brief Peripheral name */
#define BOARD_INITPINS_NAND_CSn_SIGNAL FLEXSPI_A_SS0_B   /*!<@brief Signal name */
                                                         /* @} */

/*! @name FLEXSPI0A_DATA0 (coord T5), NAND_D0
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_NAND_D0_PERIPHERAL FLEXSPI       /*!<@brief Peripheral name */
#define BOARD_INITPINS_NAND_D0_SIGNAL FLEXSPI_A_DATA0   /*!<@brief Signal name */
                                                        /* @} */

/*! @name FLEXSPI0A_DATA1 (coord U5), NAND_D1
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_NAND_D1_PERIPHERAL FLEXSPI       /*!<@brief Peripheral name */
#define BOARD_INITPINS_NAND_D1_SIGNAL FLEXSPI_A_DATA1   /*!<@brief Signal name */
                                                        /* @} */

/*! @name FLEXSPI0A_DATA2 (coord P6), NAND_D2
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_NAND_D2_PERIPHERAL FLEXSPI       /*!<@brief Peripheral name */
#define BOARD_INITPINS_NAND_D2_SIGNAL FLEXSPI_A_DATA2   /*!<@brief Signal name */
                                                        /* @} */

/*! @name FLEXSPI0A_DATA3 (coord P7), NAND_D3
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_NAND_D3_PERIPHERAL FLEXSPI       /*!<@brief Peripheral name */
#define BOARD_INITPINS_NAND_D3_SIGNAL FLEXSPI_A_DATA3   /*!<@brief Signal name */
                                                        /* @} */

/*! @name PIO0_6 (coord E1), ALS_INTn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_ALS_INTn_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_ALS_INTn_SIGNAL PIO0              /*!<@brief Signal name */
#define BOARD_INITPINS_ALS_INTn_CHANNEL 6                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_ALS_INTn_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_ALS_INTn_GPIO_PIN_MASK (1U << 6U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_ALS_INTn_PORT 0U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_ALS_INTn_PIN 6U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_ALS_INTn_PIN_MASK (1U << 6U)      /*!<@brief PORT pin mask */
                                                         /* @} */

/*! @name PIO0_14 (coord A3), DEBUG_LEDn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_DEBUG_LEDn_PERIPHERAL GPIO                    /*!<@brief Peripheral name */
#define BOARD_INITPINS_DEBUG_LEDn_SIGNAL PIO0                        /*!<@brief Signal name */
#define BOARD_INITPINS_DEBUG_LEDn_CHANNEL 14                         /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_DEBUG_LEDn_GPIO GPIO                          /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_DEBUG_LEDn_GPIO_PIN_MASK (1U << 14U)          /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_DEBUG_LEDn_PORT 0U                            /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_DEBUG_LEDn_PIN 14U                            /*!<@brief PORT pin number */
#define BOARD_INITPINS_DEBUG_LEDn_PIN_MASK (1U << 14U)               /*!<@brief PORT pin mask */
                                                                     /* @} */

/*! @name PIO0_15 (coord A5), BLE_RESETn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BLE_RESETn_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_BLE_RESETn_SIGNAL PIO0               /*!<@brief Signal name */
#define BOARD_INITPINS_BLE_RESETn_CHANNEL 15                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_BLE_RESETn_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_BLE_RESETn_GPIO_PIN_MASK (1U << 15U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_BLE_RESETn_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_BLE_RESETn_PIN 15U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_BLE_RESETn_PIN_MASK (1U << 15U)      /*!<@brief PORT pin mask */
                                                            /* @} */

/*! @name FC2_CTS_SDA_SSEL0 (coord D7), I2C5_SDA
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_I2C5_SDA_PERIPHERAL FLEXCOMM2       /*!<@brief Peripheral name */
#define BOARD_INITPINS_I2C5_SDA_SIGNAL CTS_SDA_SSEL0       /*!<@brief Signal name */
                                                           /* @} */

/*! @name FC2_RTS_SCL_SSEL1 (coord B7), I2C5_SCL
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_I2C5_SCL_PERIPHERAL FLEXCOMM2       /*!<@brief Peripheral name */
#define BOARD_INITPINS_I2C5_SCL_SIGNAL RTS_SCL_SSEL1       /*!<@brief Signal name */
                                                           /* @} */

/*! @name PIO0_19 (coord A1), BAT_INTn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BAT_INTn_PERIPHERAL PINT           /*!<@brief Peripheral name */
#define BOARD_INITPINS_BAT_INTn_SIGNAL PINT               /*!<@brief Signal name */
#define BOARD_INITPINS_BAT_INTn_CHANNEL 1                 /*!<@brief Signal channel */
#define BOARD_INITPINS_BAT_INTn_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_BAT_INTn_PIN 19U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_BAT_INTn_PIN_MASK (1U << 19U)      /*!<@brief PORT pin mask */
                                                          /* @} */

/*! @name PIO0_21 (coord C7), BAT_STATn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BAT_STATn_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_BAT_STATn_SIGNAL PIO0               /*!<@brief Signal name */
#define BOARD_INITPINS_BAT_STATn_CHANNEL 21                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_BAT_STATn_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_BAT_STATn_GPIO_PIN_MASK (1U << 21U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_BAT_STATn_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_BAT_STATn_PIN 21U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_BAT_STATn_PIN_MASK (1U << 21U)      /*!<@brief PORT pin mask */
                                                           /* @} */

/*! @name PIO0_23 (coord C9), BAT_CD
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BAT_CD_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_BAT_CD_SIGNAL PIO0               /*!<@brief Signal name */
#define BOARD_INITPINS_BAT_CD_CHANNEL 23                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_BAT_CD_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_BAT_CD_GPIO_PIN_MASK (1U << 23U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_BAT_CD_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_BAT_CD_PIN 23U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_BAT_CD_PIN_MASK (1U << 23U)      /*!<@brief PORT pin mask */
                                                        /* @} */

/*! @name FC3_CTS_SDA_SSEL0 (coord B9), I2C6_SDA
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_I2C6_SDA_PERIPHERAL FLEXCOMM3       /*!<@brief Peripheral name */
#define BOARD_INITPINS_I2C6_SDA_SIGNAL CTS_SDA_SSEL0       /*!<@brief Signal name */
                                                           /* @} */

/*! @name FC3_RTS_SCL_SSEL1 (coord A9), I2C6_SCL
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_I2C6_SCL_PERIPHERAL FLEXCOMM3       /*!<@brief Peripheral name */
#define BOARD_INITPINS_I2C6_SCL_SIGNAL RTS_SCL_SSEL1       /*!<@brief Signal name */
                                                           /* @} */

/*! @name FC4_SCK (coord D11), SSM2518_BCLK
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_SSM2518_BCLK_PERIPHERAL FLEXCOMM4 /*!<@brief Peripheral name */
#define BOARD_INITPINS_SSM2518_BCLK_SIGNAL SCK           /*!<@brief Signal name */
                                                         /* @} */

/*! @name FC4_TXD_SCL_MISO_WS (coord B10), SSM2518_LRCLK
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_SSM2518_LRCLK_PERIPHERAL FLEXCOMM4         /*!<@brief Peripheral name */
#define BOARD_INITPINS_SSM2518_LRCLK_SIGNAL TXD_SCL_MISO_WS       /*!<@brief Signal name */
                                                                  /* @} */

/*! @name FC4_RXD_SDA_MOSI_DATA (coord C11), SSM2518_SDATA
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_SSM2518_SDATA_PERIPHERAL FLEXCOMM4           /*!<@brief Peripheral name */
#define BOARD_INITPINS_SSM2518_SDATA_SIGNAL RXD_SDA_MOSI_DATA       /*!<@brief Signal name */
                                                                    /* @} */

/*! @name PIO0_31 (coord A11), MEMS_WAKE
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_MEMS_WAKE_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_MEMS_WAKE_SIGNAL PIO0               /*!<@brief Signal name */
#define BOARD_INITPINS_MEMS_WAKE_CHANNEL 31                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_MEMS_WAKE_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_MEMS_WAKE_GPIO_PIN_MASK (1U << 31U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_MEMS_WAKE_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_MEMS_WAKE_PIN 31U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_MEMS_WAKE_PIN_MASK (1U << 31U)      /*!<@brief PORT pin mask */
                                                           /* @} */

/*! @name PIO1_2 (coord A7), BLE_BOOTn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BLE_BOOTn_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_BLE_BOOTn_SIGNAL PIO1              /*!<@brief Signal name */
#define BOARD_INITPINS_BLE_BOOTn_CHANNEL 2                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_BLE_BOOTn_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_BLE_BOOTn_GPIO_PIN_MASK (1U << 2U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_BLE_BOOTn_PORT 1U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_BLE_BOOTn_PIN 2U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_BLE_BOOTn_PIN_MASK (1U << 2U)      /*!<@brief PORT pin mask */
                                                          /* @} */

/*! @name PIO1_3 (coord G16), SSM2518_SHTDNn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_SSM2518_SHTDNn_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_SSM2518_SHTDNn_SIGNAL PIO1              /*!<@brief Signal name */
#define BOARD_INITPINS_SSM2518_SHTDNn_CHANNEL 3                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_SSM2518_SHTDNn_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_SSM2518_SHTDNn_GPIO_PIN_MASK (1U << 3U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_SSM2518_SHTDNn_PORT 1U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_SSM2518_SHTDNn_PIN 3U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_SSM2518_SHTDNn_PIN_MASK (1U << 3U)      /*!<@brief PORT pin mask */
                                                               /* @} */

/*! @name FC5_TXD_SCL_MISO_WS (coord G17), DEBUG_UART_TXD
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_DEBUG_UART_TXD_PERIPHERAL FLEXCOMM5         /*!<@brief Peripheral name */
#define BOARD_INITPINS_DEBUG_UART_TXD_SIGNAL TXD_SCL_MISO_WS       /*!<@brief Signal name */
                                                                   /* @} */

/*! @name FC5_RXD_SDA_MOSI_DATA (coord J16), DEBUG_UART_RXD
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_DEBUG_UART_RXD_PERIPHERAL FLEXCOMM5           /*!<@brief Peripheral name */
#define BOARD_INITPINS_DEBUG_UART_RXD_SIGNAL RXD_SDA_MOSI_DATA       /*!<@brief Signal name */
                                                                     /* @} */

/*! @name ADC0_4 (coord B5), MEMS_ANALOG
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_MEMS_ANALOG_PERIPHERAL ADC0     /*!<@brief Peripheral name */
#define BOARD_INITPINS_MEMS_ANALOG_SIGNAL CH           /*!<@brief Signal name */
#define BOARD_INITPINS_MEMS_ANALOG_CHANNEL 4           /*!<@brief Signal channel */
                                                       /* @} */

/*! @name PIO1_6 (coord J17), PB_INTn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_PB_INTn_PERIPHERAL PINT          /*!<@brief Peripheral name */
#define BOARD_INITPINS_PB_INTn_SIGNAL PINT              /*!<@brief Signal name */
#define BOARD_INITPINS_PB_INTn_CHANNEL 2                /*!<@brief Signal channel */
#define BOARD_INITPINS_PB_INTn_PORT 1U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_PB_INTn_PIN 6U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_PB_INTn_PIN_MASK (1U << 6U)      /*!<@brief PORT pin mask */
                                                        /* @} */

/*! @name PIO1_9 (coord B1), HRM_INTn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_HRM_INTn_PERIPHERAL PINT          /*!<@brief Peripheral name */
#define BOARD_INITPINS_HRM_INTn_SIGNAL PINT              /*!<@brief Signal name */
#define BOARD_INITPINS_HRM_INTn_CHANNEL 3                /*!<@brief Signal channel */
#define BOARD_INITPINS_HRM_INTn_PORT 1U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_HRM_INTn_PIN 9U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_HRM_INTn_PIN_MASK (1U << 9U)      /*!<@brief PORT pin mask */
                                                         /* @} */

/*! @name PIO1_10 (coord K16), ACCEL_INT
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_ACCEL_INT_PERIPHERAL PINT           /*!<@brief Peripheral name */
#define BOARD_INITPINS_ACCEL_INT_SIGNAL PINT               /*!<@brief Signal name */
#define BOARD_INITPINS_ACCEL_INT_CHANNEL 4                 /*!<@brief Signal channel */
#define BOARD_INITPINS_ACCEL_INT_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_ACCEL_INT_PIN 10U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_ACCEL_INT_PIN_MASK (1U << 10U)      /*!<@brief PORT pin mask */
                                                           /* @} */

/*! @name PIO1_24 (coord T7), FLASH_MUX_SEL
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_FLASH_MUX_SEL_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_FLASH_MUX_SEL_SIGNAL PIO1               /*!<@brief Signal name */
#define BOARD_INITPINS_FLASH_MUX_SEL_CHANNEL 24                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_FLASH_MUX_SEL_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_FLASH_MUX_SEL_GPIO_PIN_MASK (1U << 24U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_FLASH_MUX_SEL_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_FLASH_MUX_SEL_PIN 24U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_FLASH_MUX_SEL_PIN_MASK (1U << 24U)      /*!<@brief PORT pin mask */
                                                               /* @} */

/*! @name PIO1_25 (coord U7), HRM_IR_ANODE_EN
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_SIGNAL PIO1               /*!<@brief Signal name */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_CHANNEL 25                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_GPIO_PIN_MASK (1U << 25U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_PIN 25U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_HRM_IR_ANODE_EN_PIN_MASK (1U << 25U)      /*!<@brief PORT pin mask */
                                                                 /* @} */

/*! @name PIO1_27 (coord T8), FLASH_MUX_EN
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_FLASH_MUX_EN_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_FLASH_MUX_EN_SIGNAL PIO1               /*!<@brief Signal name */
#define BOARD_INITPINS_FLASH_MUX_EN_CHANNEL 27                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_FLASH_MUX_EN_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_FLASH_MUX_EN_GPIO_PIN_MASK (1U << 27U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_FLASH_MUX_EN_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_FLASH_MUX_EN_PIN 27U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_FLASH_MUX_EN_PIN_MASK (1U << 27U)      /*!<@brief PORT pin mask */
                                                              /* @} */

/*! @name PIO1_30 (coord P10), HRM_RED_ANODE_EN
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_SIGNAL PIO1               /*!<@brief Signal name */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_CHANNEL 30                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_GPIO_PIN_MASK (1U << 30U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_PORT 1U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_PIN 30U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_HRM_RED_ANODE_EN_PIN_MASK (1U << 30U)      /*!<@brief PORT pin mask */
                                                                  /* @} */

/*! @name PIO2_3 (coord T12), AUX_GPIO1
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_AUX_GPIO1_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_AUX_GPIO1_SIGNAL PIO2              /*!<@brief Signal name */
#define BOARD_INITPINS_AUX_GPIO1_CHANNEL 3                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_AUX_GPIO1_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_AUX_GPIO1_GPIO_PIN_MASK (1U << 3U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_AUX_GPIO1_PORT 2U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_AUX_GPIO1_PIN 3U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_AUX_GPIO1_PIN_MASK (1U << 3U)      /*!<@brief PORT pin mask */
                                                          /* @} */

/*! @name PIO2_4 (coord T13), AUX_GPIO2
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_AUX_GPIO2_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_AUX_GPIO2_SIGNAL PIO2              /*!<@brief Signal name */
#define BOARD_INITPINS_AUX_GPIO2_CHANNEL 4                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_AUX_GPIO2_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_AUX_GPIO2_GPIO_PIN_MASK (1U << 4U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_AUX_GPIO2_PORT 2U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_AUX_GPIO2_PIN 4U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_AUX_GPIO2_PIN_MASK (1U << 4U)      /*!<@brief PORT pin mask */
                                                          /* @} */

/*! @name PIO2_6 (coord U15), AUX_GPIO3
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_AUX_GPIO3_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_AUX_GPIO3_SIGNAL PIO2              /*!<@brief Signal name */
#define BOARD_INITPINS_AUX_GPIO3_CHANNEL 6                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_AUX_GPIO3_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_AUX_GPIO3_GPIO_PIN_MASK (1U << 6U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_AUX_GPIO3_PORT 2U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_AUX_GPIO3_PIN 6U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_AUX_GPIO3_PIN_MASK (1U << 6U)      /*!<@brief PORT pin mask */
                                                          /* @} */

/*! @name PIO2_8 (coord U17), PS_HOLD
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_PS_HOLD_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_PS_HOLD_SIGNAL PIO2              /*!<@brief Signal name */
#define BOARD_INITPINS_PS_HOLD_CHANNEL 8                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_PS_HOLD_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_PS_HOLD_GPIO_PIN_MASK (1U << 8U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_PS_HOLD_PORT 2U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_PS_HOLD_PIN 8U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_PS_HOLD_PIN_MASK (1U << 8U)      /*!<@brief PORT pin mask */
                                                        /* @} */

/*! @name PIO0_16 (coord D6), USER_BUTTON1
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_USER_BUTTON1_PERIPHERAL PINT           /*!<@brief Peripheral name */
#define BOARD_INITPINS_USER_BUTTON1_SIGNAL PINT               /*!<@brief Signal name */
#define BOARD_INITPINS_USER_BUTTON1_CHANNEL 5                 /*!<@brief Signal channel */
#define BOARD_INITPINS_USER_BUTTON1_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_USER_BUTTON1_PIN 16U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_USER_BUTTON1_PIN_MASK (1U << 16U)      /*!<@brief PORT pin mask */
                                                              /* @} */

/*! @name PIO0_20 (coord B2), USER_BUTTON2
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_USER_BUTTON2_PERIPHERAL PINT           /*!<@brief Peripheral name */
#define BOARD_INITPINS_USER_BUTTON2_SIGNAL PINT               /*!<@brief Signal name */
#define BOARD_INITPINS_USER_BUTTON2_CHANNEL 6                 /*!<@brief Signal channel */
#define BOARD_INITPINS_USER_BUTTON2_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_USER_BUTTON2_PIN 20U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_USER_BUTTON2_PIN_MASK (1U << 20U)      /*!<@brief PORT pin mask */
                                                              /* @} */

/*! @name PIO0_22 (coord D8), POWER_BTNn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_POWER_BTNn_PERIPHERAL PINT           /*!<@brief Peripheral name */
#define BOARD_INITPINS_POWER_BTNn_SIGNAL PINT               /*!<@brief Signal name */
#define BOARD_INITPINS_POWER_BTNn_CHANNEL 7                 /*!<@brief Signal channel */
#define BOARD_INITPINS_POWER_BTNn_PORT 0U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_POWER_BTNn_PIN 22U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_POWER_BTNn_PIN_MASK (1U << 22U)      /*!<@brief PORT pin mask */
                                                            /* @} */

/*! @name PIO1_1 (coord G15), ISP_ENTRY
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_ISP_ENTRY_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_ISP_ENTRY_SIGNAL PIO1              /*!<@brief Signal name */
#define BOARD_INITPINS_ISP_ENTRY_CHANNEL 1                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_ISP_ENTRY_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_ISP_ENTRY_GPIO_PIN_MASK (1U << 1U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_ISP_ENTRY_PORT 1U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_ISP_ENTRY_PIN 1U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_ISP_ENTRY_PIN_MASK (1U << 1U)      /*!<@brief PORT pin mask */
                                                          /* @} */

/*! @name PIO2_13 (coord T1), MEMS_MODE
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_MEMS_MODE_PERIPHERAL GPIO           /*!<@brief Peripheral name */
#define BOARD_INITPINS_MEMS_MODE_SIGNAL PIO2               /*!<@brief Signal name */
#define BOARD_INITPINS_MEMS_MODE_CHANNEL 13                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_MEMS_MODE_GPIO GPIO                 /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_MEMS_MODE_GPIO_PIN_MASK (1U << 13U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_MEMS_MODE_PORT 2U                   /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_MEMS_MODE_PIN 13U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_MEMS_MODE_PIN_MASK (1U << 13U)      /*!<@brief PORT pin mask */
                                                           /* @} */

/*! @name FC0_TXD_SCL_MISO_WS (coord G2), BT_UART_TXD
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BT_UART_TXD_PERIPHERAL FLEXCOMM0         /*!<@brief Peripheral name */
#define BOARD_INITPINS_BT_UART_TXD_SIGNAL TXD_SCL_MISO_WS       /*!<@brief Signal name */
                                                                /* @} */

/*! @name FC0_RXD_SDA_MOSI_DATA (coord G4), BT_UART_RXD
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BT_UART_RXD_PERIPHERAL FLEXCOMM0           /*!<@brief Peripheral name */
#define BOARD_INITPINS_BT_UART_RXD_SIGNAL RXD_SDA_MOSI_DATA       /*!<@brief Signal name */
                                                                  /* @} */

/*! @name FC0_CTS_SDA_SSEL0 (coord H2), BT_UART_CTSn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BT_UART_CTSn_PERIPHERAL FLEXCOMM0       /*!<@brief Peripheral name */
#define BOARD_INITPINS_BT_UART_CTSn_SIGNAL CTS_SDA_SSEL0       /*!<@brief Signal name */
                                                               /* @} */

/*! @name FC0_RTS_SCL_SSEL1 (coord J1), BT_UART_RTSn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_BT_UART_RTSn_PERIPHERAL FLEXCOMM0       /*!<@brief Peripheral name */
#define BOARD_INITPINS_BT_UART_RTSn_SIGNAL RTS_SCL_SSEL1       /*!<@brief Signal name */
                                                               /* @} */

/*! @name FC1_SCK (coord J2), EEG_SCLK
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_SCLK_PERIPHERAL FLEXCOMM1 /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_SCLK_SIGNAL SCK           /*!<@brief Signal name */
                                                     /* @} */

/*! @name FC1_TXD_SCL_MISO_WS (coord K4), EEG_MISO
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_MISO_PERIPHERAL FLEXCOMM1         /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_MISO_SIGNAL TXD_SCL_MISO_WS       /*!<@brief Signal name */
                                                             /* @} */

/*! @name FC1_RXD_SDA_MOSI_DATA (coord L3), EEG_MOSI
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_MOSI_PERIPHERAL FLEXCOMM1           /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_MOSI_SIGNAL RXD_SDA_MOSI_DATA       /*!<@brief Signal name */
                                                               /* @} */

/*! @name FC1_CTS_SDA_SSEL0 (coord J3), EEG_CSn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_CSn_PERIPHERAL FLEXCOMM1       /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_CSn_SIGNAL CTS_SDA_SSEL0       /*!<@brief Signal name */
                                                          /* @} */

/*! @name PIO0_0 (coord G1), EEG_START
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_START_PERIPHERAL GPIO          /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_START_SIGNAL PIO0              /*!<@brief Signal name */
#define BOARD_INITPINS_EEG_START_CHANNEL 0                /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_EEG_START_GPIO GPIO                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_EEG_START_GPIO_PIN_MASK (1U << 0U) /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_EEG_START_PORT 0U                  /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_EEG_START_PIN 0U                   /*!<@brief PORT pin number */
#define BOARD_INITPINS_EEG_START_PIN_MASK (1U << 0U)      /*!<@brief PORT pin mask */
                                                          /* @} */

/*! @name PIO0_11 (coord L1), EEG_PWDn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_PWDn_PERIPHERAL GPIO                    /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_PWDn_SIGNAL PIO0                        /*!<@brief Signal name */
#define BOARD_INITPINS_EEG_PWDn_CHANNEL 11                         /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_EEG_PWDn_GPIO GPIO                          /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_EEG_PWDn_GPIO_PIN_MASK (1U << 11U)          /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_EEG_PWDn_PORT 0U                            /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_EEG_PWDn_PIN 11U                            /*!<@brief PORT pin number */
#define BOARD_INITPINS_EEG_PWDn_PIN_MASK (1U << 11U)               /*!<@brief PORT pin mask */
                                                                   /* @} */

/*! @name ADC0_0 (coord F4), TEMP_SENSE_BAND
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_TEMP_SENSE_BAND_PERIPHERAL ADC0         /*!<@brief Peripheral name */
#define BOARD_INITPINS_TEMP_SENSE_BAND_SIGNAL CH               /*!<@brief Signal name */
#define BOARD_INITPINS_TEMP_SENSE_BAND_CHANNEL 0               /*!<@brief Signal channel */
                                                               /* @} */

/*! @name PIO1_7 (coord J15), EEG_RESETn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_RESETn_PERIPHERAL GPIO                    /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_RESETn_SIGNAL PIO1                        /*!<@brief Signal name */
#define BOARD_INITPINS_EEG_RESETn_CHANNEL 7                          /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_EEG_RESETn_GPIO GPIO                          /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_EEG_RESETn_GPIO_PIN_MASK (1U << 7U)           /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_EEG_RESETn_PORT 1U                            /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_EEG_RESETn_PIN 7U                             /*!<@brief PORT pin number */
#define BOARD_INITPINS_EEG_RESETn_PIN_MASK (1U << 7U)                /*!<@brief PORT pin mask */
                                                                     /* @} */

/*! @name PIO2_7 (coord U16), EEG_LDO_EN
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_LDO_EN_PERIPHERAL GPIO                    /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_LDO_EN_SIGNAL PIO2                        /*!<@brief Signal name */
#define BOARD_INITPINS_EEG_LDO_EN_CHANNEL 7                          /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_EEG_LDO_EN_GPIO GPIO                          /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_EEG_LDO_EN_GPIO_PIN_MASK (1U << 7U)           /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_EEG_LDO_EN_PORT 2U                            /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_EEG_LDO_EN_PIN 7U                             /*!<@brief PORT pin number */
#define BOARD_INITPINS_EEG_LDO_EN_PIN_MASK (1U << 7U)                /*!<@brief PORT pin mask */
                                                                     /* @} */

/*! @name SCT0_OUT2 (coord E3), LEDB_PWM
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_LEDB_PWM_PERIPHERAL SCT0    /*!<@brief Peripheral name */
#define BOARD_INITPINS_LEDB_PWM_SIGNAL OUT         /*!<@brief Signal name */
#define BOARD_INITPINS_LEDB_PWM_CHANNEL 2          /*!<@brief Signal channel */
                                                   /* @} */

/*! @name SCT0_OUT6 (coord A2), LEDG_PWM
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_LEDG_PWM_PERIPHERAL SCT0    /*!<@brief Peripheral name */
#define BOARD_INITPINS_LEDG_PWM_SIGNAL OUT         /*!<@brief Signal name */
#define BOARD_INITPINS_LEDG_PWM_CHANNEL 6          /*!<@brief Signal channel */
                                                   /* @} */

/*! @name SCT0_OUT7 (coord B3), LEDR_PWM
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_LEDR_PWM_PERIPHERAL SCT0    /*!<@brief Peripheral name */
#define BOARD_INITPINS_LEDR_PWM_SIGNAL OUT         /*!<@brief Signal name */
#define BOARD_INITPINS_LEDR_PWM_CHANNEL 7          /*!<@brief Signal channel */
                                                   /* @} */

/*! @name PIO0_13 (coord G3), EEG_DRDYn
  @{ */
/* Routed pin properties */
#define BOARD_INITPINS_EEG_DRDYn_PERIPHERAL GPIO                   /*!<@brief Peripheral name */
#define BOARD_INITPINS_EEG_DRDYn_SIGNAL PIO0                       /*!<@brief Signal name */
#define BOARD_INITPINS_EEG_DRDYn_CHANNEL 13                        /*!<@brief Signal channel */

/* Symbols to be used with GPIO driver */
#define BOARD_INITPINS_EEG_DRDYn_GPIO GPIO                         /*!<@brief GPIO peripheral base pointer */
#define BOARD_INITPINS_EEG_DRDYn_GPIO_PIN_MASK (1U << 13U)         /*!<@brief GPIO pin mask */
#define BOARD_INITPINS_EEG_DRDYn_PORT 0U                           /*!<@brief PORT peripheral base pointer */
#define BOARD_INITPINS_EEG_DRDYn_PIN 13U                           /*!<@brief PORT pin number */
#define BOARD_INITPINS_EEG_DRDYn_PIN_MASK (1U << 13U)              /*!<@brief PORT pin mask */
                                                                   /* @} */

/*!
 * @brief 
 *
 */
void BOARD_InitPins(void); /* Function assigned for the Cortex-M33 */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif /* _PIN_MUX_H_ */

/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/
