/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Peripherals v11.0
processor: MIMXRT685S
package_id: MIMXRT685SFVKB
mcu_data: ksdk2_0
processor_version: 12.0.1
functionalGroups:
- name: BOARD_InitPeripherals
  UUID: fda78168-3fc8-40df-80b9-53d6edf10537
  called_from_default_init: true
  selectedCore: cm33
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
component:
- type: 'system'
- type_id: 'system_54b53072540eeeb8f8e9343e71f28176'
- global_system_definitions:
  - user_definitions: ''
  - user_includes: ''
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
component:
- type: 'uart_cmsis_common'
- type_id: 'uart_cmsis_common_9cb8e302497aa696fdbb5a4fd622c2a8'
- global_USART_CMSIS_common:
  - quick_selection: 'default'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
component:
- type: 'gpio_adapter_common'
- type_id: 'gpio_adapter_common_57579b9ac814fe26bf95df0a384c36b6'
- global_gpio_adapter_common:
  - quick_selection: 'default'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/***********************************************************************************************************************
 * Included files
 **********************************************************************************************************************/
#include "peripherals.h"

/***********************************************************************************************************************
 * BOARD_InitPeripherals functional group
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * DMA0 initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'DMA0'
- type: 'lpc_dma'
- mode: 'basic'
- custom_name_enabled: 'false'
- type_id: 'lpc_dma_c13ca997a68f2ca6c666916ba13db7d7'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'DMA0'
- config_sets:
  - fsl_dma:
    - dma_table:
      - 0: []
      - 1: []
    - dma_channels: []
    - init_interrupt: 'false'
    - dma_interrupt:
      - IRQn: 'DMA0_IRQn'
      - enable_interrrupt: 'enabled'
      - enable_priority: 'false'
      - priority: '0'
      - enable_custom_name: 'false'
    - quick_selection: 'default'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/* Empty initialization function (commented out)
static void DMA0_init(void) {
} */

/***********************************************************************************************************************
 * NVIC initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'NVIC'
- type: 'nvic'
- mode: 'general'
- custom_name_enabled: 'false'
- type_id: 'nvic_57b5eef3774cc60acaede6f5b8bddc67'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'NVIC'
- config_sets:
  - nvic:
    - interrupt_table:
      - 0: []
      - 1: []
      - 2: []
      - 3: []
      - 4: []
    - interrupts: []
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/* Empty initialization function (commented out)
static void NVIC_init(void) {
} */

/***********************************************************************************************************************
 * FC2_BATT_I2C initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FC2_BATT_I2C'
- type: 'flexcomm_i2c'
- mode: 'freertos'
- custom_name_enabled: 'true'
- type_id: 'flexcomm_i2c_c8597948f61bd571ab263ea4330b9dd6'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXCOMM2'
- config_sets:
  - fsl_i2c:
    - i2c_mode: 'kI2C_Master'
    - clockSource: 'FXCOMFunctionClock'
    - clockSourceFreq: 'BOARD_BootClockRUN'
    - rtos_handle:
      - enable_custom_name: 'false'
    - i2c_master_config:
      - enableMaster: 'true'
      - baudRate_Bps: '100000'
      - enableTimeout: 'false'
      - timeout_Ms: '35'
    - interrupt_priority:
      - IRQn: 'FLEXCOMM2_IRQn'
      - enable_priority: 'true'
      - priority: '5'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
i2c_rtos_handle_t FC2_BATT_I2C_rtosHandle;
const i2c_master_config_t FC2_BATT_I2C_config = {
  .enableMaster = true,
  .baudRate_Bps = 100000UL,
  .enableTimeout = false,
  .timeout_Ms = 35U
};

static void FC2_BATT_I2C_init(void) {
  /* Initialization function */
  I2C_RTOS_Init(&FC2_BATT_I2C_rtosHandle, FC2_BATT_I2C_PERIPHERAL, &FC2_BATT_I2C_config, FC2_BATT_I2C_CLOCK_SOURCE);
  /* Interrupt vector FLEXCOMM2_IRQn priority settings in the NVIC. */
  NVIC_SetPriority(FC2_BATT_I2C_FLEXCOMM_IRQN, FC2_BATT_I2C_FLEXCOMM_IRQ_PRIORITY);
}

/***********************************************************************************************************************
 * FC3_SENSOR_I2C initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FC3_SENSOR_I2C'
- type: 'flexcomm_i2c'
- mode: 'freertos'
- custom_name_enabled: 'true'
- type_id: 'flexcomm_i2c_c8597948f61bd571ab263ea4330b9dd6'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXCOMM3'
- config_sets:
  - fsl_i2c:
    - i2c_mode: 'kI2C_Master'
    - clockSource: 'FXCOMFunctionClock'
    - clockSourceFreq: 'BOARD_BootClockRUN'
    - rtos_handle:
      - enable_custom_name: 'false'
    - i2c_master_config:
      - enableMaster: 'true'
      - baudRate_Bps: '100000'
      - enableTimeout: 'false'
      - timeout_Ms: '35'
    - interrupt_priority:
      - IRQn: 'FLEXCOMM3_IRQn'
      - enable_priority: 'false'
      - priority: '0'
    - quick_selection: 'QS_I2C_Master'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
i2c_rtos_handle_t FC3_SENSOR_I2C_rtosHandle;
const i2c_master_config_t FC3_SENSOR_I2C_config = {
  .enableMaster = true,
  .baudRate_Bps = 100000UL,
  .enableTimeout = false,
  .timeout_Ms = 35U
};

static void FC3_SENSOR_I2C_init(void) {
  /* Initialization function */
  I2C_RTOS_Init(&FC3_SENSOR_I2C_rtosHandle, FC3_SENSOR_I2C_PERIPHERAL, &FC3_SENSOR_I2C_config, FC3_SENSOR_I2C_CLOCK_SOURCE);
}

/***********************************************************************************************************************
 * FC5_DEBUG_UART initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FC5_DEBUG_UART'
- type: 'flexcomm_usart'
- mode: 'freertos'
- custom_name_enabled: 'true'
- type_id: 'flexcomm_usart_ed63debb5f147199906723fc49ad2e72'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXCOMM5'
- config_sets:
  - fsl_usart_freertos:
    - usart_rtos_configuration:
      - clockSource: 'FXCOMFunctionClock'
      - clockSourceFreq: 'BOARD_BootClockRUN'
      - baudrate: '115200'
      - parity: 'kUSART_ParityDisabled'
      - stopbits: 'kUSART_OneStopBit'
      - buffer_size: '256'
    - interrupt_priority:
      - IRQn: 'FLEXCOMM5_IRQn'
      - enable_priority: 'false'
      - priority: '0'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
usart_rtos_handle_t FC5_DEBUG_UART_rtos_handle;
usart_handle_t FC5_DEBUG_UART_usart_handle;
uint8_t FC5_DEBUG_UART_background_buffer[FC5_DEBUG_UART_BACKGROUND_BUFFER_SIZE];
struct rtos_usart_config FC5_DEBUG_UART_rtos_config = {
  .base = FC5_DEBUG_UART_PERIPHERAL,
  .baudrate = 115200UL,
  .parity = kUSART_ParityDisabled,
  .stopbits = kUSART_OneStopBit,
  .buffer = FC5_DEBUG_UART_background_buffer,
  .buffer_size = FC5_DEBUG_UART_BACKGROUND_BUFFER_SIZE
};

static void FC5_DEBUG_UART_init(void) {
  /* USART clock source initialization */
  FC5_DEBUG_UART_rtos_config.srcclk = FC5_DEBUG_UART_CLOCK_SOURCE;
  /* USART rtos initialization */
  USART_RTOS_Init(&FC5_DEBUG_UART_rtos_handle, &FC5_DEBUG_UART_usart_handle, &FC5_DEBUG_UART_rtos_config);
}

/***********************************************************************************************************************
 * PINT initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'PINT'
- type: 'pint'
- mode: 'interrupt_mode'
- custom_name_enabled: 'false'
- type_id: 'pint_cf4a806bb2a6c1ffced58ae2ed7b43af'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'PINT'
- config_sets:
  - general:
    - interrupt_array:
      - 0:
        - interrupt_id: 'INT_1'
        - interrupt_selection: 'PINT.1'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'charger_pint_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT1_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 1:
        - interrupt_id: 'INT_2'
        - interrupt_selection: 'PINT.2'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'pb_int_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT2_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 2:
        - interrupt_id: 'INT_3'
        - interrupt_selection: 'PINT.3'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'hrm_pint_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT3_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 3:
        - interrupt_id: 'INT_4'
        - interrupt_selection: 'PINT.4'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'accel_pint_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT4_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 4:
        - interrupt_id: 'INT_5'
        - interrupt_selection: 'PINT.5'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'user_button1_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT5_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 5:
        - interrupt_id: 'INT_6'
        - interrupt_selection: 'PINT.6'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'user_button2_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT6_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 6:
        - interrupt_id: 'INT_7'
        - interrupt_selection: 'PINT.7'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'power_button_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT7_IRQn'
          - enable_priority: 'false'
          - priority: '0'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

static void PINT_init(void) {
  /* PINT initiation  */
  PINT_Init(PINT_PERIPHERAL);
  /* PINT PINT.1 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_1, kPINT_PinIntEnableNone, charger_pint_isr);
  /* PINT PINT.2 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_2, kPINT_PinIntEnableNone, pb_int_isr);
  /* PINT PINT.3 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_3, kPINT_PinIntEnableNone, hrm_pint_isr);
  /* PINT PINT.4 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_4, kPINT_PinIntEnableNone, accel_pint_isr);
  /* PINT PINT.5 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_5, kPINT_PinIntEnableNone, user_button1_isr);
  /* PINT PINT.6 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_6, kPINT_PinIntEnableNone, user_button2_isr);
  /* PINT PINT.7 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_7, kPINT_PinIntEnableNone, power_button_isr);
}

/***********************************************************************************************************************
 * SCT0_LED initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'SCT0_LED'
- type: 'sctimer'
- mode: 'basic'
- custom_name_enabled: 'true'
- type_id: 'sctimer_7973000102117ff9c4fa4742aaf3ccb0'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'SCT0'
- config_sets:
  - main:
    - config:
      - clockMode: 'kSCTIMER_System_ClockMode'
      - clockSource: 'SynchronousFunctionClock'
      - clockSourceFreq: 'GetFreq'
      - SCTInputClockSourceFreq: 'custom:0'
      - clockSelect: 'kSCTIMER_Clock_On_Rise_Input_0'
      - enableCounterUnify: 'true'
      - enableBidirection_l: 'false'
      - enableBidirection_h: 'false'
      - prescale_l: '1'
      - prescale_h: '1'
      - outInitState: ''
      - inputsync: ''
    - enableIRQ: 'true'
    - interrupt:
      - IRQn: 'SCT0_IRQn'
      - enable_interrrupt: 'enabled'
      - enable_priority: 'true'
      - priority: '5'
      - enable_custom_name: 'false'
    - enableLTimer: 'false'
    - enableHTimer: 'false'
    - pwms:
      - 0:
        - output: 'kSCTIMER_Out_2'
        - level: 'kSCTIMER_HighTrue'
        - dutyCyclePercent: '1'
      - 1:
        - output: 'kSCTIMER_Out_6'
        - level: 'kSCTIMER_HighTrue'
        - dutyCyclePercent: '1'
      - 2:
        - output: 'kSCTIMER_Out_7'
        - level: 'kSCTIMER_HighTrue'
        - dutyCyclePercent: '1'
    - pwmMode: 'kSCTIMER_EdgeAlignedPwm'
    - pwmFrequency: '25000'
    - events: []
    - states:
      - 0:
        - pwms: 'pwm0 pwm1 pwm2'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
const sctimer_config_t SCT0_LED_initConfig = {
  .enableCounterUnify = true,
  .clockMode = kSCTIMER_System_ClockMode,
  .clockSelect = kSCTIMER_Clock_On_Rise_Input_0,
  .enableBidirection_l = false,
  .enableBidirection_h = false,
  .prescale_l = 0U,
  .prescale_h = 0U,
  .outInitState = 0U,
  .inputsync = 0U
};
const sctimer_pwm_signal_param_t SCT0_LED_pwmSignalsConfig[3] = {
  {
    .output = kSCTIMER_Out_2,
    .level = kSCTIMER_HighTrue,
    .dutyCyclePercent = 1U
  },
  {
    .output = kSCTIMER_Out_6,
    .level = kSCTIMER_HighTrue,
    .dutyCyclePercent = 1U
  },
  {
    .output = kSCTIMER_Out_7,
    .level = kSCTIMER_HighTrue,
    .dutyCyclePercent = 1U
  }
};
uint32_t SCT0_LED_pwmEvent[3];

static void SCT0_LED_init(void) {
  SCTIMER_Init(SCT0_LED_PERIPHERAL, &SCT0_LED_initConfig);
  /* Initialization of state 0 */
  SCTIMER_SetupPwm(SCT0_LED_PERIPHERAL, &SCT0_LED_pwmSignalsConfig[0], kSCTIMER_EdgeAlignedPwm, 25000U, SCT0_LED_CLOCK_FREQ, &SCT0_LED_pwmEvent[0]);
  SCTIMER_SetupPwm(SCT0_LED_PERIPHERAL, &SCT0_LED_pwmSignalsConfig[1], kSCTIMER_EdgeAlignedPwm, 25000U, SCT0_LED_CLOCK_FREQ, &SCT0_LED_pwmEvent[1]);
  SCTIMER_SetupPwm(SCT0_LED_PERIPHERAL, &SCT0_LED_pwmSignalsConfig[2], kSCTIMER_EdgeAlignedPwm, 25000U, SCT0_LED_CLOCK_FREQ, &SCT0_LED_pwmEvent[2]);
  /* Interrupt vector SCT0_IRQn priority settings in the NVIC. */
  NVIC_SetPriority(SCT0_LED_IRQN, SCT0_LED_IRQ_PRIORITY);
  /* Enable interrupt SCT0_IRQn request in the NVIC. */
  EnableIRQ(SCT0_LED_IRQN);
}

/***********************************************************************************************************************
 * FC0_BLE_UART initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FC0_BLE_UART'
- type: 'flexcomm_usart'
- mode: 'freertos'
- custom_name_enabled: 'true'
- type_id: 'flexcomm_usart_ed63debb5f147199906723fc49ad2e72'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXCOMM0'
- config_sets:
  - fsl_usart_freertos:
    - usart_rtos_configuration:
      - clockSource: 'FXCOMFunctionClock'
      - clockSourceFreq: 'BOARD_BootClockRUN'
      - baudrate: '115200'
      - parity: 'kUSART_ParityDisabled'
      - stopbits: 'kUSART_OneStopBit'
      - buffer_size: '2048'
    - interrupt_priority:
      - IRQn: 'FLEXCOMM0_IRQn'
      - enable_priority: 'true'
      - priority: '5'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
usart_rtos_handle_t FC0_BLE_UART_rtos_handle;
usart_handle_t FC0_BLE_UART_usart_handle;
uint8_t FC0_BLE_UART_background_buffer[FC0_BLE_UART_BACKGROUND_BUFFER_SIZE];
struct rtos_usart_config FC0_BLE_UART_rtos_config = {
  .base = FC0_BLE_UART_PERIPHERAL,
  .baudrate = 115200UL,
  .parity = kUSART_ParityDisabled,
  .stopbits = kUSART_OneStopBit,
  .buffer = FC0_BLE_UART_background_buffer,
  .buffer_size = FC0_BLE_UART_BACKGROUND_BUFFER_SIZE
};

static void FC0_BLE_UART_init(void) {
  /* USART clock source initialization */
  FC0_BLE_UART_rtos_config.srcclk = FC0_BLE_UART_CLOCK_SOURCE;
  /* USART rtos initialization */
  USART_RTOS_Init(&FC0_BLE_UART_rtos_handle, &FC0_BLE_UART_usart_handle, &FC0_BLE_UART_rtos_config);
  /* Interrupt vector FLEXCOMM0_IRQn priority settings in the NVIC. */
  NVIC_SetPriority(FC0_BLE_UART_IRQN, FC0_BLE_UART_IRQ_PRIORITY);
}

/***********************************************************************************************************************
 * FC1_EEG_SPI initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FC1_EEG_SPI'
- type: 'flexcomm_spi'
- mode: 'dma'
- custom_name_enabled: 'true'
- type_id: 'flexcomm_spi_481dadba00035f986f31ed9ac95af181'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXCOMM1'
- config_sets:
  - fsl_spi:
    - spi_mode: 'kSPI_Master'
    - clockSource: 'FXCOMFunctionClock'
    - clockSourceFreq: 'BOARD_BootClockRUN'
    - spi_master_config:
      - enableLoopback: 'false'
      - enableMaster: 'true'
      - polarity: 'kSPI_ClockPolarityActiveHigh'
      - phase: 'kSPI_ClockPhaseSecondEdge'
      - direction: 'kSPI_MsbFirst'
      - baudRate_Bps: '2000000'
      - dataWidth: 'kSPI_Data8Bits'
      - sselNum: 'kSPI_Ssel0'
      - sselPol_set: ''
      - txWatermark: 'kSPI_TxFifo0'
      - rxWatermark: 'kSPI_RxFifo1'
      - delayConfig:
        - preDelay: '1'
        - postDelay: '5'
        - frameDelay: '0'
        - transferDelay: '0'
  - dmaCfg:
    - dma_channels:
      - enable_rx_dma_channel: 'true'
      - dma_rx_channel:
        - DMA_source: 'kDma0RequestFlexcomm1Rx'
        - init_channel_priority: 'false'
        - dma_priority: 'kDMA_ChannelPriority0'
        - enable_custom_name: 'false'
      - enable_tx_dma_channel: 'true'
      - dma_tx_channel:
        - DMA_source: 'kDma0RequestFlexcomm1Tx'
        - init_channel_priority: 'false'
        - dma_priority: 'kDMA_ChannelPriority0'
        - enable_custom_name: 'false'
    - spi_dma_handle:
      - enable_custom_name: 'false'
      - init_callback: 'true'
      - callback_fcn: 'eeg_dma_rx_complete_isr'
      - user_data: ''
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
const spi_master_config_t FC1_EEG_SPI_config = {
  .enableLoopback = false,
  .enableMaster = true,
  .polarity = kSPI_ClockPolarityActiveHigh,
  .phase = kSPI_ClockPhaseSecondEdge,
  .direction = kSPI_MsbFirst,
  .baudRate_Bps = 2000000UL,
  .dataWidth = kSPI_Data8Bits,
  .sselNum = kSPI_Ssel0,
  .sselPol = kSPI_SpolActiveAllLow,
  .txWatermark = kSPI_TxFifo0,
  .rxWatermark = kSPI_RxFifo1,
  .delayConfig = {
    .preDelay = 1U,
    .postDelay = 5U,
    .frameDelay = 0U,
    .transferDelay = 0U
  }
};
dma_handle_t FC1_EEG_SPI_RX_Handle;
dma_handle_t FC1_EEG_SPI_TX_Handle;
spi_dma_handle_t FC1_EEG_SPI_DMA_Handle;

static void FC1_EEG_SPI_init(void) {
  /* Initialization function */
  SPI_MasterInit(FC1_EEG_SPI_PERIPHERAL, &FC1_EEG_SPI_config, FC1_EEG_SPI_CLOCK_SOURCE);
  /* Enable the DMA 2 channel in the DMA */
  DMA_EnableChannel(FC1_EEG_SPI_RX_DMA_BASEADDR, FC1_EEG_SPI_RX_DMA_CHANNEL);
  /* Create the DMA FC1_EEG_SPI_RX_Handle handle */
  DMA_CreateHandle(&FC1_EEG_SPI_RX_Handle, FC1_EEG_SPI_RX_DMA_BASEADDR, FC1_EEG_SPI_RX_DMA_CHANNEL);
  /* Enable the DMA 3 channel in the DMA */
  DMA_EnableChannel(FC1_EEG_SPI_TX_DMA_BASEADDR, FC1_EEG_SPI_TX_DMA_CHANNEL);
  /* Create the DMA FC1_EEG_SPI_TX_Handle handle */
  DMA_CreateHandle(&FC1_EEG_SPI_TX_Handle, FC1_EEG_SPI_TX_DMA_BASEADDR, FC1_EEG_SPI_TX_DMA_CHANNEL);
  /* Create the SPI DMA handle */
  SPI_MasterTransferCreateHandleDMA(FC1_EEG_SPI_PERIPHERAL, &FC1_EEG_SPI_DMA_Handle, eeg_dma_rx_complete_isr, NULL, &FC1_EEG_SPI_TX_Handle, &FC1_EEG_SPI_RX_Handle);
}

/***********************************************************************************************************************
 * Initialization functions
 **********************************************************************************************************************/
void BOARD_InitPeripherals(void)
{
  /* Global initialization */
  DMA_Init(DMA0_DMA_BASEADDR);

  /* Initialize components */
  FC2_BATT_I2C_init();
  FC3_SENSOR_I2C_init();
  FC5_DEBUG_UART_init();
  PINT_init();
  SCT0_LED_init();
  FC0_BLE_UART_init();
  FC1_EEG_SPI_init();
}

/***********************************************************************************************************************
 * BOARD_InitBootPeripherals function
 **********************************************************************************************************************/
void BOARD_InitBootPeripherals(void)
{
  BOARD_InitPeripherals();
}
