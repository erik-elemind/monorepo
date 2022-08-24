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
processor_version: 12.0.0
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
      - 2: []
      - 3: []
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
    - interrupts: []
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/* Empty initialization function (commented out)
static void NVIC_init(void) {
} */

/***********************************************************************************************************************
 * FLEXCOMM0_EEG_SPI initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FLEXCOMM0_EEG_SPI'
- type: 'flexcomm_spi'
- mode: 'dma'
- custom_name_enabled: 'true'
- type_id: 'flexcomm_spi_481dadba00035f986f31ed9ac95af181'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXCOMM0'
- config_sets:
  - fsl_spi:
    - spi_mode: 'kSPI_Master'
    - clockSource: 'FXCOMFunctionClock'
    - clockSourceFreq: 'BOARD_BootClockRUN'
    - spi_master_config:
      - enableLoopback: 'false'
      - enableMaster: 'true'
      - polarity: 'kSPI_ClockPolarityActiveHigh'
      - phase: 'kSPI_ClockPhaseFirstEdge'
      - direction: 'kSPI_MsbFirst'
      - baudRate_Bps: '500000'
      - dataWidth: 'kSPI_Data8Bits'
      - sselNum: 'kSPI_Ssel0'
      - sselPol_set: ''
      - txWatermark: 'kSPI_TxFifo0'
      - rxWatermark: 'kSPI_RxFifo1'
      - delayConfig:
        - preDelay: '0'
        - postDelay: '0'
        - frameDelay: '0'
        - transferDelay: '0'
  - dmaCfg:
    - dma_channels:
      - enable_rx_dma_channel: 'true'
      - dma_rx_channel:
        - DMA_source: 'kDma0RequestFlexcomm0Rx'
        - init_channel_priority: 'false'
        - dma_priority: 'kDMA_ChannelPriority0'
        - enable_custom_name: 'false'
      - enable_tx_dma_channel: 'true'
      - dma_tx_channel:
        - DMA_source: 'kDma0RequestFlexcomm0Tx'
        - init_channel_priority: 'false'
        - dma_priority: 'kDMA_ChannelPriority0'
        - enable_custom_name: 'false'
    - spi_dma_handle:
      - enable_custom_name: 'false'
      - init_callback: 'false'
      - callback_fcn: ''
      - user_data: ''
    - quick_selection: 'QuickSelection1'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
const spi_master_config_t FLEXCOMM0_EEG_SPI_config = {
  .enableLoopback = false,
  .enableMaster = true,
  .polarity = kSPI_ClockPolarityActiveHigh,
  .phase = kSPI_ClockPhaseFirstEdge,
  .direction = kSPI_MsbFirst,
  .baudRate_Bps = 500000UL,
  .dataWidth = kSPI_Data8Bits,
  .sselNum = kSPI_Ssel0,
  .sselPol = kSPI_SpolActiveAllLow,
  .txWatermark = kSPI_TxFifo0,
  .rxWatermark = kSPI_RxFifo1,
  .delayConfig = {
    .preDelay = 0U,
    .postDelay = 0U,
    .frameDelay = 0U,
    .transferDelay = 0U
  }
};
dma_handle_t FLEXCOMM0_EEG_SPI_RX_Handle;
dma_handle_t FLEXCOMM0_EEG_SPI_TX_Handle;
spi_dma_handle_t FLEXCOMM0_EEG_SPI_DMA_Handle;

static void FLEXCOMM0_EEG_SPI_init(void) {
  /* Initialization function */
  SPI_MasterInit(FLEXCOMM0_EEG_SPI_PERIPHERAL, &FLEXCOMM0_EEG_SPI_config, FLEXCOMM0_EEG_SPI_CLOCK_SOURCE);
  /* Enable the DMA 0 channel in the DMA */
  DMA_EnableChannel(FLEXCOMM0_EEG_SPI_RX_DMA_BASEADDR, FLEXCOMM0_EEG_SPI_RX_DMA_CHANNEL);
  /* Create the DMA FLEXCOMM0_EEG_SPI_RX_Handle handle */
  DMA_CreateHandle(&FLEXCOMM0_EEG_SPI_RX_Handle, FLEXCOMM0_EEG_SPI_RX_DMA_BASEADDR, FLEXCOMM0_EEG_SPI_RX_DMA_CHANNEL);
  /* Enable the DMA 1 channel in the DMA */
  DMA_EnableChannel(FLEXCOMM0_EEG_SPI_TX_DMA_BASEADDR, FLEXCOMM0_EEG_SPI_TX_DMA_CHANNEL);
  /* Create the DMA FLEXCOMM0_EEG_SPI_TX_Handle handle */
  DMA_CreateHandle(&FLEXCOMM0_EEG_SPI_TX_Handle, FLEXCOMM0_EEG_SPI_TX_DMA_BASEADDR, FLEXCOMM0_EEG_SPI_TX_DMA_CHANNEL);
  /* Create the SPI DMA handle */
  SPI_MasterTransferCreateHandleDMA(FLEXCOMM0_EEG_SPI_PERIPHERAL, &FLEXCOMM0_EEG_SPI_DMA_Handle, NULL, NULL, &FLEXCOMM0_EEG_SPI_TX_Handle, &FLEXCOMM0_EEG_SPI_RX_Handle);
}

/***********************************************************************************************************************
 * FLEXSPI initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FLEXSPI'
- type: 'flexspi'
- mode: 'dma'
- custom_name_enabled: 'false'
- type_id: 'flexspi_cc6da638fb0490ad15096647c2b8e52a'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXSPI'
- config_sets:
  - fsl_flexspi:
    - flexspiConfig:
      - rxSampleClock: 'kFLEXSPI_ReadSampleClkLoopbackInternally'
      - clockSource: 'FlexSpiClock'
      - clockSourceFreq: 'BOARD_BootClockRUN'
      - enableSckFreeRunning: 'false'
      - enableDoze: 'false'
      - enableHalfSpeedAccess: 'false'
      - enableSckBDiffOpt: 'false'
      - enableSameConfigForAll: 'false'
      - seqTimeoutCycleString: '65535'
      - ipGrantTimeoutCycleString: '255'
      - txWatermark: '8'
      - rxWatermark: '8'
      - ahbConfig:
        - ahbGrantTimeoutCycleString: '255'
        - ahbBusTimeoutCycleString: '65535'
        - resumeWaitCycleString: '32'
        - buffer:
          - 0:
            - priority: '0'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
          - 1:
            - priority: '1'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
          - 2:
            - priority: '2'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
          - 3:
            - priority: '3'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
          - 4:
            - priority: '4'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
          - 5:
            - priority: '5'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
          - 6:
            - priority: '6'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
          - 7:
            - priority: '7'
            - masterIndex: '0'
            - bufferSize: '256'
            - enablePrefetch: 'true'
        - enableClearAHBBufferOpt: 'false'
        - enableReadAddressOpt: 'false'
        - enableAHBPrefetch: 'false'
        - enableAHBBufferable: 'false'
        - enableAHBCachable: 'false'
    - flexspiInterrupt:
      - interrupt_sel: ''
    - enableCustomLUT: 'true'
    - lutConfig:
      - flash: 'customFlash'
      - lutName: 'defaultLUT'
      - lutSizeCustom: '64'
    - devices_configs: []
    - dma_channels:
      - enable_rx_dma_channel: 'true'
      - dma_rx_channel:
        - DMA_source: 'kDma0RequestNoDMARequest28'
        - init_channel_priority: 'false'
        - dma_priority: 'kDMA_ChannelPriority0'
        - enable_custom_name: 'false'
      - enable_tx_dma_channel: 'true'
      - dma_tx_channel:
        - DMA_source: 'kDma0RequestNoDMARequest29'
        - init_channel_priority: 'false'
        - dma_priority: 'kDMA_ChannelPriority0'
        - enable_custom_name: 'false'
    - spi_dma_handle:
      - enable_custom_name: 'false'
      - init_callback_dma: 'false'
      - callback_fcn_dma: ''
      - user_data_dma: ''
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
const flexspi_config_t FLEXSPI_config = {
  .rxSampleClock = kFLEXSPI_ReadSampleClkLoopbackInternally,
  .enableSckFreeRunning = false,
  .enableDoze = false,
  .enableHalfSpeedAccess = false,
  .enableSckBDiffOpt = false,
  .enableSameConfigForAll = false,
  .seqTimeoutCycle = 65535,
  .ipGrantTimeoutCycle = 255,
  .txWatermark = 8U,
  .rxWatermark = 8U,
  .ahbConfig = {
    .ahbGrantTimeoutCycle = 255,
    .ahbBusTimeoutCycle = 65535,
    .resumeWaitCycle = 32,
    .buffer = {
      {
        .priority = 0,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      },
      {
        .priority = 1,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      },
      {
        .priority = 2,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      },
      {
        .priority = 3,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      },
      {
        .priority = 4,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      },
      {
        .priority = 5,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      },
      {
        .priority = 6,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      },
      {
        .priority = 7,
        .masterIndex = 0U,
        .bufferSize = 256U,
        .enablePrefetch = true
      }
    },
    .enableClearAHBBufferOpt = false,
    .enableReadAddressOpt = false,
    .enableAHBPrefetch = false,
    .enableAHBBufferable = false,
    .enableAHBCachable = false
  }
};
dma_handle_t FLEXSPI_TX_Handle;
dma_handle_t FLEXSPI_RX_Handle;
flexspi_dma_handle_t FLEXSPI_DMA_Handle;

static void FLEXSPI_init(void) {
  /* FLEXSPI peripheral initialization */
  FLEXSPI_Init(FLEXSPI_PERIPHERAL, &FLEXSPI_config);
  /* Update LUT table. */
  FLEXSPI_UpdateLUT(FLEXSPI_PERIPHERAL, 0, FLEXSPI_LUT, FLEXSPI_LUT_LENGTH);
  /* Enable the DMA 29 channel in the DMA */
  DMA_EnableChannel(FLEXSPI_TX_DMA_BASEADDR, FLEXSPI_TX_DMA_CHANNEL);
  /* Enable the DMA 28 channel in the DMA */
  DMA_EnableChannel(FLEXSPI_RX_DMA_BASEADDR, FLEXSPI_RX_DMA_CHANNEL);
  /* Create the DMA FLEXSPI_TX_Handle handle */
  DMA_CreateHandle(&FLEXSPI_TX_Handle, FLEXSPI_TX_DMA_BASEADDR, FLEXSPI_TX_DMA_CHANNEL);
  /* Create the DMA FLEXSPI_RX_Handle handle */
  DMA_CreateHandle(&FLEXSPI_RX_Handle, FLEXSPI_RX_DMA_BASEADDR, FLEXSPI_RX_DMA_CHANNEL);
  /* Initializes the FLEXSPI DMA handle which is used in transactional functions. */
  FLEXSPI_TransferCreateHandleDMA(FLEXSPI_PERIPHERAL, &FLEXSPI_DMA_Handle, NULL, NULL, &FLEXSPI_TX_Handle, &FLEXSPI_RX_Handle);
}

/***********************************************************************************************************************
 * FLEXCOMM1_BLE_UART initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FLEXCOMM1_BLE_UART'
- type: 'flexcomm_usart'
- mode: 'freertos'
- custom_name_enabled: 'true'
- type_id: 'flexcomm_usart_ed63debb5f147199906723fc49ad2e72'
- functional_group: 'BOARD_InitPeripherals'
- peripheral: 'FLEXCOMM1'
- config_sets:
  - fsl_usart_freertos:
    - usart_rtos_configuration:
      - clockSource: 'FXCOMFunctionClock'
      - clockSourceFreq: 'BOARD_BootClockRUN'
      - baudrate: '115200'
      - parity: 'kUSART_ParityDisabled'
      - stopbits: 'kUSART_OneStopBit'
      - buffer_size: '1'
    - interrupt_priority:
      - IRQn: 'FLEXCOMM1_IRQn'
      - enable_priority: 'false'
      - priority: '0'
    - quick_selection: 'QuickSelection1'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
usart_rtos_handle_t FLEXCOMM1_BLE_UART_rtos_handle;
usart_handle_t FLEXCOMM1_BLE_UART_usart_handle;
uint8_t FLEXCOMM1_BLE_UART_background_buffer[FLEXCOMM1_BLE_UART_BACKGROUND_BUFFER_SIZE];
struct rtos_usart_config FLEXCOMM1_BLE_UART_rtos_config = {
  .base = FLEXCOMM1_BLE_UART_PERIPHERAL,
  .baudrate = 115200UL,
  .parity = kUSART_ParityDisabled,
  .stopbits = kUSART_OneStopBit,
  .buffer = FLEXCOMM1_BLE_UART_background_buffer,
  .buffer_size = FLEXCOMM1_BLE_UART_BACKGROUND_BUFFER_SIZE
};

static void FLEXCOMM1_BLE_UART_init(void) {
  /* USART clock source initialization */
  FLEXCOMM1_BLE_UART_rtos_config.srcclk = FLEXCOMM1_BLE_UART_CLOCK_SOURCE;
  /* USART rtos initialization */
  USART_RTOS_Init(&FLEXCOMM1_BLE_UART_rtos_handle, &FLEXCOMM1_BLE_UART_usart_handle, &FLEXCOMM1_BLE_UART_rtos_config);
}

/***********************************************************************************************************************
 * FLEXCOMM2_BATT_I2C initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FLEXCOMM2_BATT_I2C'
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
      - enable_priority: 'false'
      - priority: '0'
    - quick_selection: 'QS_I2C_Master'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
i2c_rtos_handle_t FLEXCOMM2_BATT_I2C_rtosHandle;
const i2c_master_config_t FLEXCOMM2_BATT_I2C_config = {
  .enableMaster = true,
  .baudRate_Bps = 100000UL,
  .enableTimeout = false,
  .timeout_Ms = 35U
};

static void FLEXCOMM2_BATT_I2C_init(void) {
  /* Initialization function */
  I2C_RTOS_Init(&FLEXCOMM2_BATT_I2C_rtosHandle, FLEXCOMM2_BATT_I2C_PERIPHERAL, &FLEXCOMM2_BATT_I2C_config, FLEXCOMM2_BATT_I2C_CLOCK_SOURCE);
}

/***********************************************************************************************************************
 * FLEXCOMM3_SENSOR_I2C initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FLEXCOMM3_SENSOR_I2C'
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
i2c_rtos_handle_t FLEXCOMM3_SENSOR_I2C_rtosHandle;
const i2c_master_config_t FLEXCOMM3_SENSOR_I2C_config = {
  .enableMaster = true,
  .baudRate_Bps = 100000UL,
  .enableTimeout = false,
  .timeout_Ms = 35U
};

static void FLEXCOMM3_SENSOR_I2C_init(void) {
  /* Initialization function */
  I2C_RTOS_Init(&FLEXCOMM3_SENSOR_I2C_rtosHandle, FLEXCOMM3_SENSOR_I2C_PERIPHERAL, &FLEXCOMM3_SENSOR_I2C_config, FLEXCOMM3_SENSOR_I2C_CLOCK_SOURCE);
}

/***********************************************************************************************************************
 * FLEXCOMM5_DEBUG_UART initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FLEXCOMM5_DEBUG_UART'
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
      - buffer_size: '1'
    - interrupt_priority:
      - IRQn: 'FLEXCOMM5_IRQn'
      - enable_priority: 'false'
      - priority: '0'
    - quick_selection: 'QuickSelection1'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
usart_rtos_handle_t FLEXCOMM5_DEBUG_UART_rtos_handle;
usart_handle_t FLEXCOMM5_DEBUG_UART_usart_handle;
uint8_t FLEXCOMM5_DEBUG_UART_background_buffer[FLEXCOMM5_DEBUG_UART_BACKGROUND_BUFFER_SIZE];
struct rtos_usart_config FLEXCOMM5_DEBUG_UART_rtos_config = {
  .base = FLEXCOMM5_DEBUG_UART_PERIPHERAL,
  .baudrate = 115200UL,
  .parity = kUSART_ParityDisabled,
  .stopbits = kUSART_OneStopBit,
  .buffer = FLEXCOMM5_DEBUG_UART_background_buffer,
  .buffer_size = FLEXCOMM5_DEBUG_UART_BACKGROUND_BUFFER_SIZE
};

static void FLEXCOMM5_DEBUG_UART_init(void) {
  /* USART clock source initialization */
  FLEXCOMM5_DEBUG_UART_rtos_config.srcclk = FLEXCOMM5_DEBUG_UART_CLOCK_SOURCE;
  /* USART rtos initialization */
  USART_RTOS_Init(&FLEXCOMM5_DEBUG_UART_rtos_handle, &FLEXCOMM5_DEBUG_UART_usart_handle, &FLEXCOMM5_DEBUG_UART_rtos_config);
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
        - interrupt_id: 'INT_0'
        - interrupt_selection: 'PINT.0'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: ''
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT0_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 1:
        - interrupt_id: 'INT_1'
        - interrupt_selection: 'PINT.1'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: ''
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT1_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 2:
        - interrupt_id: 'INT_2'
        - interrupt_selection: 'PINT.2'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: ''
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT2_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 3:
        - interrupt_id: 'INT_3'
        - interrupt_selection: 'PINT.3'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: 'hrm_pint_isr'
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT3_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 4:
        - interrupt_id: 'INT_4'
        - interrupt_selection: 'PINT.4'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: ''
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT4_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 5:
        - interrupt_id: 'INT_5'
        - interrupt_selection: 'PINT.5'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: ''
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT5_IRQn'
          - enable_priority: 'false'
          - priority: '0'
      - 6:
        - interrupt_id: 'INT_6'
        - interrupt_selection: 'PINT.6'
        - interrupt_type: 'kPINT_PinIntEnableNone'
        - callback_function: ''
        - enable_callback: 'false'
        - interrupt:
          - IRQn: 'PIN_INT6_IRQn'
          - enable_priority: 'false'
          - priority: '0'
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

static void PINT_init(void) {
  /* PINT initiation  */
  PINT_Init(PINT_PERIPHERAL);
  /* PINT PINT.0 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_0, kPINT_PinIntEnableNone, NULL);
  /* PINT PINT.1 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_1, kPINT_PinIntEnableNone, NULL);
  /* PINT PINT.2 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_2, kPINT_PinIntEnableNone, NULL);
  /* PINT PINT.3 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_3, kPINT_PinIntEnableNone, hrm_pint_isr);
  /* PINT PINT.4 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_4, kPINT_PinIntEnableNone, NULL);
  /* PINT PINT.5 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_5, kPINT_PinIntEnableNone, NULL);
  /* PINT PINT.6 configuration */
  PINT_PinInterruptConfig(PINT_PERIPHERAL, PINT_INT_6, kPINT_PinIntEnableNone, NULL);
}

/***********************************************************************************************************************
 * Initialization functions
 **********************************************************************************************************************/
void BOARD_InitPeripherals(void)
{
  /* Global initialization */
  DMA_Init(DMA0_DMA_BASEADDR);

  /* Initialize components */
  FLEXCOMM0_EEG_SPI_init();
  FLEXSPI_init();
  FLEXCOMM1_BLE_UART_init();
  FLEXCOMM2_BATT_I2C_init();
  FLEXCOMM3_SENSOR_I2C_init();
  FLEXCOMM5_DEBUG_UART_init();
  PINT_init();
}

/***********************************************************************************************************************
 * BOARD_InitBootPeripherals function
 **********************************************************************************************************************/
void BOARD_InitBootPeripherals(void)
{
  BOARD_InitPeripherals();
}
