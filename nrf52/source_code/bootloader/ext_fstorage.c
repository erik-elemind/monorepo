/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: June, 2021
 * Author:  Paul Adelsbach
 *
 * Description: External flash storage interface.
 */

#include "ext_fstorage.h"
#include "ext_flash.h"
#include "nrf_delay.h"

// nordic log
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_LEVEL       (4)
#define NRF_LOG_MODULE_NAME EF
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Define a macro to get the number of elements in an array
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// Flag to run the test cases. This is set in the build scripts.
// Must have RTT enabled with logs to observe the results.
#ifdef EXT_FSTORAGE_TEST
STATIC_ASSERT(NRF_LOG_ENABLED);
#include "ext_fstorage_test.h"
#endif

// Callback to upper layer
static nrf_fstorage_evt_handler_t m_handler;

// Blocking delay for complete of write (or erase)
static ret_code_t wait_for_write_complete(uint32_t ms)
{
    static const uint32_t delay_step_ms = 1;
    uint32_t ms_elapsed = 0;
    ret_code_t err_code;
    nand_status_reg_t status;

    do
    {
        err_code = ext_flash_cmd_status_read(&status, status_only_reg_addr);
        if (NRF_SUCCESS != err_code)
        {
            return NRF_ERROR_INTERNAL;
        }

        if (0 == status.raw)
        {
            //NRF_LOG_INFO("wip cleared. ms_elapsed=%d (of %d)", ms_elapsed, ms);
            return NRF_SUCCESS;
        }

        // Busy wait
        nrf_delay_ms(delay_step_ms);
        ms_elapsed += delay_step_ms;
    } while(ms_elapsed < ms);

    NRF_LOG_WARNING("timeout. ms_elapsed=%d", ms_elapsed);

    return NRF_ERROR_TIMEOUT;
}

ret_code_t ext_fstorage_init(nrf_fstorage_evt_handler_t evt_handler)
{
    // Cache the callback provided by the caller.
    m_handler = evt_handler;

    ret_code_t err_code = ext_flash_init();
    if (NRF_SUCCESS != err_code)
    {
        return err_code;
    }

    #ifdef EXT_FSTORAGE_TEST
    // Null the handler for the test to avoid any unintended effects from
    // callbacks in the upper layers.
    nrf_fstorage_evt_handler_t handler_orig = m_handler;
    m_handler = NULL;
    ext_fstorage_test();
    m_handler = handler_orig;
    #endif

    return NRF_SUCCESS;
}

ret_code_t ext_fstorage_read(uint32_t               addr,
                             void                 * p_dest,
                             uint32_t               len)
{
    ret_code_t err_code;
    uint32_t len_cmd;

    while (len > 0)
    {
        // Load page to cache
        err_code = ext_flash_cmd_read_page(addr);
        if (NRF_SUCCESS != err_code)
        {
            return NRF_ERROR_INTERNAL;
        }

        // Wait for complete
        err_code = wait_for_write_complete(SPI_FLASH_TIMEOUT_PAGE_PROG);
        if (NRF_SUCCESS != err_code)
        {
            return err_code;
        }

        // Read data off cache
        len_cmd = len;
        err_code = ext_flash_cmd_read_mem(addr, &len_cmd, p_dest);
        if (NRF_SUCCESS != err_code)
        {
            return NRF_ERROR_INTERNAL;
        }

        addr += 1; // minimum read size is 1 page, functions here are in page_addrs
        //addr += len_cmd;
        p_dest = (void*)((uint32_t)p_dest + len_cmd);
        len -= len_cmd;
    }

    // NOTE: this function does not call the callback to match the analogous 
    //       routine, nrf_fstorage_read

    return NRF_SUCCESS;
}

ret_code_t ext_fstorage_write(uint32_t               dest,
                              void           const * p_src,
                              uint32_t               len,
                              void                 * p_param,
                              bool                   notify)
{
    ret_code_t err_code;

    nrf_fstorage_evt_t e = {
        .id         = NRF_FSTORAGE_EVT_WRITE_RESULT,
        .result     = NRF_SUCCESS,
        .addr       = 0,
        .p_src      = p_src,
        .len        = len,
        .p_param    = p_param,
    };

    if (notify) goto end;

    while (len > 0)
    {
        // Enable writes
        err_code = ext_flash_cmd_write_enable();
        if (NRF_SUCCESS != err_code)
        {
            return NRF_ERROR_INTERNAL;
        }

        // Load Program Data
        uint32_t len_cmd = len;
        err_code = ext_flash_cmd_load_program(dest, &len_cmd, p_src);
        if (NRF_SUCCESS != err_code)
        {
            return NRF_ERROR_INTERNAL;
        }

        // Execute Program Data
        err_code = ext_flash_cmd_page_program(dest);
        if (NRF_SUCCESS != err_code)
        {
            return NRF_ERROR_INTERNAL;
        }

        // Wait for complete
        err_code = wait_for_write_complete(SPI_FLASH_TIMEOUT_PAGE_PROG);
        if (NRF_SUCCESS != err_code)
        {
            return err_code;
        }

        NRF_LOG_INFO("write mem. addr=%06X", dest);

        dest += 1; // addresses are by pages 
        //dest += len_cmd;
        p_src = (void*)((uint32_t)p_src + len_cmd);
        len -= len_cmd;
    }
    return NRF_SUCCESS;

    end:
    // For review:
    // It may be preferrable to call the handler via the scheduler, as 
    // a deferred call.
    // As shown below, the callback is invoked prior to the original 
    // function returning. This does not cause a problem in initial testing.
    if (m_handler)
    {
        m_handler(&e);
    }

    return NRF_SUCCESS;
}

ret_code_t ext_fstorage_erase(uint32_t page_addr,
                              uint32_t num_blocks,
                              void   * p_param,
                              bool     notify)
{
    ret_code_t err_code;

    nrf_fstorage_evt_t e = {
        .id         = NRF_FSTORAGE_EVT_ERASE_RESULT,
        .result     = NRF_SUCCESS,
        .addr       = page_addr,
        .p_src      = (void*)page_addr,
        .len        = num_blocks,
        .p_param    = p_param,
    };

    if (notify) goto end;

    for (uint32_t i=0; i<num_blocks; i++, page_addr += SPI_FLASH_PAGES_IN_BLOCK)
    {
        // Enable writes
        err_code = ext_flash_cmd_write_enable();
        if (NRF_SUCCESS != err_code)
        {
            return NRF_ERROR_INTERNAL;
        }

        // Erase the current block
        err_code = ext_flash_cmd_block_erase(page_addr);
        if (NRF_SUCCESS != err_code)
        {
            NRF_LOG_WARNING("block erase failed. addr=0x%08X", page_addr);
            return NRF_ERROR_INTERNAL;
        }

        // Wait for complete
        err_code = wait_for_write_complete(SPI_FLASH_TIMEOUT_SECTOR_ERASE);
        if (NRF_SUCCESS != err_code)
        {
            return err_code;
        }
    }
    return NRF_SUCCESS;

    end:
    if (m_handler)
    {
        m_handler(&e);
    }

    return NRF_SUCCESS;
}

uint32_t ext_fstorage_page_count(uint32_t addr,
                                 uint32_t len)
{
    uint32_t num_pages = 0;
    uint32_t chunk_len;
    
    // First chunk can be misaligned
    chunk_len = SPI_FLASH_BLOCK_LEN - (addr & (SPI_FLASH_BLOCK_LEN-1));

    while (len > 0)
    {
        len -= MIN(len, chunk_len);
        num_pages++;
        chunk_len = SPI_FLASH_BLOCK_LEN;
    }

    NRF_LOG_INFO("num_pages: %d", num_pages);

    return num_pages;
}

uint32_t ext_fstorage_block_count(uint32_t page_addr,
                                 uint32_t len)
{
    uint32_t num_pages = 0;
    uint32_t num_blocks = 0;
    uint32_t chunk_len;
    
    // First chunk can be misaligned
    chunk_len = SPI_FLASH_BLOCK_LEN - (page_addr & (SPI_FLASH_BLOCK_LEN-1));

    while (len > 0)
    {
        len -= MIN(len, chunk_len);
        num_pages++;
        chunk_len = SPI_FLASH_BLOCK_LEN;
    }

    num_blocks = (num_pages / SPI_FLASH_PAGES_IN_BLOCK); // 0 -> 1 block; 1.1 -> 2 blocks; 0.9 -> 1 block; 1 -> 1 block
    NRF_LOG_INFO("num_blocks: %d", num_blocks);

    return (num_blocks > 1) ? num_blocks+1 : 1;
}
