/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: LPC interface for applying FW updates.
 *              This layer acts as routing between the LPC MCU bootloader 
 *              interface, NRF UART, and external flash.
 */

#include "lpc_protocol.h"
#include "ext_fstorage.h"
#include "lpc_reset_timing.h"
#include "ext_flash.h"

// nordic sdk
#include "nrf_dfu_validation.h"
#include "nrfx_uart.h"
#include "boards.h"
#include "sdk_config.h"
#include "nrf_delay.h"

// nordic log
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_LEVEL       (4)
#define NRF_LOG_MODULE_NAME LPU
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// UART instance which is connected to the LPC
static const nrfx_uart_t m_uart_instance = NRFX_UART_INSTANCE(0);

// Reset the LPC, and optionally enter in-system programming mode (ISP). 
// This routine mirrors on_write_lpc_reset() in ble_elemind.c. 
// Any updates here should be considered for that function as well.
static void lpc_reset_isp(bool isp)
{
    // Set up LPC reset and ISP GPIOs (active-low)
    if (isp)
    {
        nrf_gpio_pin_clear(ISP0N_PIN);
        nrf_gpio_pin_set(ISP1N_PIN);
        nrf_gpio_pin_set(ISP2N_PIN);
        nrf_gpio_cfg_output(LPC_ISPN_PIN);
    }

    nrf_gpio_pin_clear(LPC_RESETN_PIN);
    nrf_gpio_cfg_output(LPC_RESETN_PIN);
    // Hold reset low for a little bit
    nrf_delay_us(LPC_RESETN_HOLD_US); 
    nrf_gpio_cfg_input(LPC_RESETN_PIN, NRF_GPIO_PIN_PULLUP);

    if (isp)
    {
        // Hold ISP enter flag low for a bit more
        nrf_delay_ms(LPC_ISPN_HOLD_TIME_MS);
        nrf_gpio_pin_clear(ISP0N_PIN);
        nrf_gpio_pin_set(ISP1N_PIN);
        nrf_gpio_pin_clear(ISP2N_PIN);
    }
}

// This function is invoked by the NRF bootloader following successful download
// of the 'external' applcation file.
nrf_dfu_result_t nrf_dfu_validation_post_external_app_execute(dfu_init_command_t const * p_init, bool is_trusted)
{
    // Init UART
    static const nrfx_uart_config_t uart_cfg = {
        .pseltxd    = TX_PIN_NUMBER,
        .pselrxd    = RX_PIN_NUMBER,
        .pselcts    = NRF_UART_PSEL_DISCONNECTED,
        .pselrts    = NRF_UART_PSEL_DISCONNECTED,
        .hwfc       = NRF_UART_HWFC_DISABLED,
        .parity     = NRF_UART_PARITY_EXCLUDED,
        // The LPC protocol interface is unreliable above 230k baud. 
        // This may be due to the polling/blocking implementation.
        // To solve this completely, we would likely need to use the libuarte 
        // module, which requires timers, PPI, etc that is not ideal for the 
        // bootloader.
        .baudrate   = NRF_UART_BAUDRATE_230400,
        .interrupt_priority = NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY,
    };
    nrfx_err_t err = nrfx_uart_init(&m_uart_instance, &uart_cfg, NULL);

    // Per the documentation for nrfx_uart_rx(), RX enable must be called 
    // to prevent the receive operation from stopping when an error is 
    // encountered. We will observe timeout 'errors' (RXTO), so we must 
    // call this function before kicking off a receive.
    (void)nrfx_uart_rx_enable(&m_uart_instance);

    if (NRFX_SUCCESS == err)
    {
        // Enter ISP
        lpc_reset_isp(true);

        // Init the lower layer
        if (0 == lpc_protocol_init())
        {
            // NRF_LOG_INFO("reading properties");
            // lpc_property_read();

            // Finally, apply the update to the LPC
            NRF_LOG_WARNING("apply lpc app. size=%d", p_init->app_size);
            if (0 == lpc_protocol_apply_fw(p_init->app_size))
            {
                // Success! LPC reboot initiated.
                NRF_LOG_WARNING("lpc firmware applied successfully!");
                //lpc_reset_isp(false);
                return NRF_DFU_RES_CODE_SUCCESS;
            }
        }
        else
        {
            NRF_LOG_WARNING("failed to init lpc interface");
        }

        // Upon error, reset the LPC to exit ISP
        lpc_reset_isp(false);
    }
    else
    {
        NRF_LOG_WARNING("failed to init uart");
    }

    return NRF_DFU_RES_CODE_OPERATION_FAILED;
}

// Implementation of the function needed by lpc interface code
int lpc_fw_file_read(uint32_t offset, uint8_t* p_buf, uint32_t len)
{
    ret_code_t err_code;
    // TODO: NEED TO ADJUST FOR PAGE READS....
    static uint8_t read_buf[SPI_FLASH_PAGE_LEN];
    static uint8_t *read_ptr = read_buf;
    static uint8_t running_read = 0;

    uint32_t num_pages = offset / SPI_FLASH_PAGE_LEN;
    uint32_t adjusted_dest = EXT_STORAGE_ADDR_NEW_BASE + num_pages;

    // 4 chunks in page
    if (((offset % SPI_FLASH_PAGE_LEN) == 0) || (offset == 0))
    {
        err_code = ext_fstorage_read(adjusted_dest, read_buf, SPI_FLASH_PAGE_LEN);
        if (NRF_SUCCESS != err_code)
        {
            return -1;
        }
        read_ptr = &read_buf[0];
        running_read = 0;      
    }

    memcpy(p_buf, read_ptr, len);
    read_ptr += len;
    running_read += len;

    return (int)err_code;
}

// Implementation of the function needed by lpc interface code
int lpc_uart_read_byte(uint8_t* byte) 
{
    return (int)nrfx_uart_rx(&m_uart_instance, byte, 1);
}

// Implementation of the function needed by lpc interface code
int lpc_uart_write(const uint8_t* buf, uint32_t len)
{
    return (int)nrfx_uart_tx(&m_uart_instance, buf, len);
}