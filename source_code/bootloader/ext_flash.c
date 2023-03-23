/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: June, 2021
 * Author:  Paul Adelsbach
 *
 * Description: External flash low level interface.
 */

#include "ext_flash.h"
#include "nrfx_spi.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_sdh.h" // for nrf_sdh_is_enabled()
#include "boards.h"

// nordic log
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_LEVEL       (4)
#define NRF_LOG_MODULE_NAME EXT
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

static const nrfx_spi_config_t m_spi_cfg = 
{
    .sck_pin = SPIM0_SCK_PIN,
    .mosi_pin = SPIM0_MOSI_PIN,
    .miso_pin = SPIM0_MISO_PIN,

    // CS pin is controlled manually, outside of nrfx_spi.
    // This allows for more granular control over each SPI transaction, 
    // so that this module can utilize callers buffers directly in small 
    // groups of transactions, whereas nrfx_spi would deassert CS between them.
    .ss_pin = NRFX_SPI_PIN_NOT_USED,

    // Attempt to match SD event priority 
    // https://devzone.nordicsemi.com/f/nordic-q-a/68938/softdevice-s113-interrupt-priority-levels-0-1-and-4-or-6
    .irq_priority = APP_IRQ_PRIORITY_LOW,
    
    .orc = 0xff,
    .frequency = NRF_SPI_FREQ_4M,
    .mode = NRF_SPI_MODE_0,
    .bit_order = NRF_SPI_BIT_ORDER_MSB_FIRST,
};
static const nrfx_spi_t m_spi_instance = NRFX_SPI_INSTANCE(0);

#define SPI_FLASH_OPCODE_LOAD_PROGRAM       0x02    // load program data buffer
#define SPI_FLASH_OPCODE_PAGE_PROGRAM       0x10    // write program data buffer to memory
#define SPI_FLASH_OPCODE_NORMAL_READ        0x03    // read data from flash
#define SPI_FLASH_OPCODE_READ_STATUS        0x0F    // read status register. (0x05 also works)
#define SPI_FLASH_OPCODE_WRITE_ENABLE       0x06    // write enable. must be sent prior to load_program_data, page_program or block_erase
#define SPI_FLASH_OPCODE_BLOCK_ERASE        0xD8    // erases 64 pages (128kb)
#define SPI_FLASH_OPCODE_CHIP_ERASE         0x60    // 0x60 or 0xC7
#define SPI_FLASH_OPCODE_DEVICE_ID          0x90    // device manufacturer ID and device ID register
#define SPI_FLASH_OPCODE_READ_ID            0x9F    // read jedec stats: mfg id, mem type, and density

// Convert a 32bit little endian address into 3 address bytes in the 
// format expected by the SPI chip.
static void addr_to_be24(uint32_t addr, uint8_t addr_be[3])
{
    addr_be[0] = (addr >> 16) & 0xff;
    addr_be[1] = (addr >>  8) & 0xff;
    addr_be[2] = (addr >>  0) & 0xff;
}

// Send a group of transfers, while keeping CS asserted for all of them.
static nrfx_err_t ext_flash_cmd_send(const nrfx_spi_xfer_desc_t desc[], uint32_t desc_num)
{
    nrfx_err_t err = NRFX_SUCCESS;

    nrf_gpio_pin_clear(SPIM0_CSN_PIN);
    for (uint32_t i=0; i<desc_num && NRFX_SUCCESS == err; i++)
    {
        err = nrfx_spi_xfer(&m_spi_instance, &desc[i], 0);
    }
    nrf_gpio_pin_set(SPIM0_CSN_PIN);

    return err;
}

ret_code_t ext_flash_cmd_read_id(void)
{
    static const uint8_t tx_buf[] = {SPI_FLASH_OPCODE_READ_ID};
    static uint8_t rx_buf[5];
    static const nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TRX(tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf))
    };

    nrfx_err_t err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("read id failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    NRF_LOG_INFO("read id. mfgid=0x%02X, deviceID[0]=0x%02X, deviceID[1]=0x%02X", 
        rx_buf[2],
        rx_buf[3],
        rx_buf[4]);

    return NRF_SUCCESS;
}

ret_code_t ext_flash_cmd_read_mem(const uint32_t addr, uint32_t* len, uint8_t* data)
{
    nrfx_err_t err;
    uint32_t len_cmd;

    if (*len == 0) 
    {
        return NRF_SUCCESS;
    }

    // cmd is 1 byte opcode, plus 3 bytes big endian address (MSB first)
    uint8_t tx_buf[] = {SPI_FLASH_OPCODE_NORMAL_READ,0,0,0};
    addr_to_be24(addr, &tx_buf[1]);

    // Prevent wrap
    len_cmd = SPI_FLASH_PAGE_LEN - (addr & (SPI_FLASH_PAGE_LEN-1));
    len_cmd = MIN(len_cmd, *len);

    // Perform two tranfsers while keeping the CS pin asserted so that 
    // we can utilize the callers buffer directly.
    nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TX(tx_buf, sizeof(tx_buf)),
        NRFX_SPI_XFER_RX(data, len_cmd),
    };
    err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("read mem failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    NRF_LOG_INFO("read mem. addr=0x%06X, len=%d/%d, data=0x%02X", 
        addr,
        len_cmd,
        *len,
        data[0]);

    *len = len_cmd;

    return NRF_SUCCESS;
}

ret_code_t ext_flash_cmd_write_enable(void)
{
    static const uint8_t tx_buf[] = {SPI_FLASH_OPCODE_WRITE_ENABLE};

    nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TX(tx_buf, sizeof(tx_buf)),
    };
    nrfx_err_t err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("wren failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    return NRF_SUCCESS;
}

ret_code_t ext_flash_cmd_status_read(nand_status_reg_t* status, uint8_t addr)
{
    // cmd is 1 byte opcode, followed by an 8-bit status reg addr
    uint8_t tx_buf[] = {SPI_FLASH_OPCODE_READ_STATUS, addr};

    // Perform two transfer while keeping the CS pin asserted so that 
    // we can utilize the callers buffer directly.
    nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TX(tx_buf, sizeof(tx_buf)),
        NRFX_SPI_XFER_RX(&status->raw, sizeof(*status)),
    };
    nrfx_err_t err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("status read failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    return NRF_SUCCESS;
}

ret_code_t ext_flash_cmd_chip_erase(void)
{
    // cmd is 1 byte opcode only
    static const uint8_t tx_buf[] = {SPI_FLASH_OPCODE_CHIP_ERASE};

    // Single transfer. Caller must wait for completion by checking status.
    nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TX(tx_buf, sizeof(tx_buf)),
    };
    nrfx_err_t err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("chip erase failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    NRF_LOG_INFO("chip erase initiated.");

    return NRF_SUCCESS;
}

ret_code_t ext_flash_cmd_block_erase(uint32_t addr)
{
    // cmd is 1 byte opcode, plus 3 bytes big endian address (MSB first)
    uint8_t tx_buf[] = {SPI_FLASH_OPCODE_BLOCK_ERASE,0,0,0};
    addr_to_be24(addr, &tx_buf[1]);

    // Single transfer. Caller must wait for completion by checking status.
    nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TX(tx_buf, sizeof(tx_buf)),
    };
    nrfx_err_t err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("block erase failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    NRF_LOG_INFO("block erase. addr=0x%06X", addr);

    return NRF_SUCCESS;
}

ret_code_t ext_flash_cmd_load_program(uint32_t addr, uint32_t* len, const uint8_t* data)
{
    uint32_t len_cmd;

    if (*len == 0)
    {
        return NRF_SUCCESS;
    }

    // CMD is 1 byte opcode, plus 16-bit Column Address followed by the data (2kb max)
    uint8_t tx_buf[] = {SPI_FLASH_OPCODE_LOAD_PROGRAM, 0, 0};
    
    // Handle wrapping case
    len_cmd = SPI_FLASH_PAGE_LEN - (addr & (SPI_FLASH_PAGE_LEN-1));
    len_cmd = MIN(len_cmd, *len);

    // Perform two tranfsers while keeping the CS pin asserted so that 
    // we can utilize the callers buffer directly.
    nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TX(tx_buf, sizeof(tx_buf)),
        NRFX_SPI_XFER_TX(data, len_cmd),
    };

    nrfx_err_t err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("program load failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    NRF_LOG_INFO("page prog. addr=0x%06X, len=%d/%d, data=0x%02x", 
        addr, 
        len_cmd,
        *len,
        data[0]);

    *len = len_cmd;
    return NRF_SUCCESS;
}

ret_code_t ext_flash_cmd_page_program(uint32_t addr)
{

    // cmd is 1 byte opcode, plus 24-bit big endian address (MSB first)
    uint8_t tx_buf[] = {SPI_FLASH_OPCODE_PAGE_PROGRAM,0,0,0};
    addr_to_be24(addr, &tx_buf[1]);

    // Perform two tranfsers while keeping the CS pin asserted so that 
    // we can utilize the callers buffer directly.
    nrfx_spi_xfer_desc_t desc[] = {
        NRFX_SPI_XFER_TX(tx_buf, sizeof(tx_buf))
    };
    nrfx_err_t err = ext_flash_cmd_send(desc, ARRAY_SIZE(desc));
    if (NRFX_SUCCESS != err)
    {
        NRF_LOG_WARNING("page prog failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    return NRF_SUCCESS;
}

ret_code_t ext_flash_init(void)
{
    // Initialize SPI in blocking mode
    nrfx_err_t err = nrfx_spi_init(&m_spi_instance, &m_spi_cfg, NULL, NULL);
    if (NRFX_SUCCESS == err)
    {
        // This is first init of the SPI flash.
        nrf_gpio_cfg(SPIM0_CSN_PIN,
                    NRF_GPIO_PIN_DIR_OUTPUT,
                    NRF_GPIO_PIN_INPUT_CONNECT,
                    NRF_GPIO_PIN_NOPULL,
                    NRF_GPIO_PIN_S0S1,
                    NRF_GPIO_PIN_NOSENSE);

        // Past applications restarted Flash, comment out for now pending design
        // nrf_gpio_cfg(SPI_FLASH_RSTN,
        //             NRF_GPIO_PIN_DIR_OUTPUT,
        //             NRF_GPIO_PIN_INPUT_CONNECT,
        //             NRF_GPIO_PIN_NOPULL,
        //             NRF_GPIO_PIN_S0S1,
        //             NRF_GPIO_PIN_NOSENSE);

        // // Reset the SPI flash to clear state.
        // // Spec says min reset hold time is 10us (tRLRH)
        // nrf_gpio_pin_clear(SPI_FLASH_RSTN);
        // nrf_delay_us(100);
        // nrf_gpio_pin_set(SPI_FLASH_RSTN);
        // nrf_delay_us(100);
    }
    else if (NRFX_ERROR_INVALID_STATE == err)
    {
        NRF_LOG_INFO("spi already initialized. sd_enabled=%d", nrf_sdh_is_enabled());
    }
    else
    {
        NRF_LOG_WARNING("spi init failed. code=%d", err);
        return NRF_ERROR_INTERNAL;
    }

    return NRF_SUCCESS;
}
