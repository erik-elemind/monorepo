/**
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "nrf_dfu_flash.h"
#include "nrf_dfu_types.h"

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "nrf_fstorage_nvmc.h"

#include "ext_fstorage.h"
#include "ext_flash.h"

#define NRF_LOG_MODULE_NAME nrf_dfu_flash
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

void dfu_fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

NRF_FSTORAGE_DEF(nrf_fstorage_t m_fs) =
{
    .evt_handler = dfu_fstorage_evt_handler,
    .start_addr  = MBR_SIZE,
    .end_addr    = BOOTLOADER_SETTINGS_ADDRESS + BOOTLOADER_SETTINGS_PAGE_SIZE
};

static uint32_t m_flash_operations_pending;

void dfu_fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (NRF_LOG_ENABLED && (m_flash_operations_pending > 0))
    {
        m_flash_operations_pending--;
    }

    if (p_evt->result == NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Flash %s success: addr=%p, pending %d",
                      (p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT) ? "write" : "erase",
                      p_evt->addr, m_flash_operations_pending);
    }
    else
    {
        NRF_LOG_DEBUG("Flash %s failed (0x%x): addr=%p, len=0x%x bytes, pending %d",
                      (p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT) ? "write" : "erase",
                      p_evt->result, p_evt->addr, p_evt->len, m_flash_operations_pending);
    }

    if (p_evt->p_param)
    {
        //lint -save -e611 (Suspicious cast)
        ((nrf_dfu_flash_callback_t)(p_evt->p_param))((void*)p_evt->p_src);
        //lint -restore
    }
}


ret_code_t nrf_dfu_flash_init(bool sd_irq_initialized)
{
    nrf_fstorage_api_t * p_api_impl;
    ret_code_t err_code;

    /* Setup the desired API implementation. */
#if defined(BLE_STACK_SUPPORT_REQD) || defined(ANT_STACK_SUPPORT_REQD)
    if (sd_irq_initialized)
    {
        NRF_LOG_DEBUG("Initializing nrf_fstorage_sd backend.");
        p_api_impl = &nrf_fstorage_sd;
    }
    else
#endif
    {
        NRF_LOG_DEBUG("Initializing nrf_fstorage_nvmc backend.");
        p_api_impl = &nrf_fstorage_nvmc;
    }

    err_code = nrf_fstorage_init(&m_fs, p_api_impl, NULL);
    if (NRF_SUCCESS != err_code)
    {
        return err_code;
    }

    return ext_fstorage_init(dfu_fstorage_evt_handler);
}


ret_code_t nrf_dfu_flash_store(uint32_t                   dest,
                               void               const * p_src,
                               uint32_t                   len,
                               nrf_dfu_flash_callback_t   callback)
{
    static uint8_t buffer[SPI_FLASH_PAGE_LEN];
    static uint32_t buffer_offset = 0;
    uint8_t *p_src_ptr = p_src;

    ret_code_t rc;

    if (EXT_STORAGE_IS_ADDR(dest))
    {
        uint32_t relative_byte_address = dest - EXT_STORAGE_ADDR_BASE;
        uint32_t num_pages = relative_byte_address / SPI_FLASH_PAGE_LEN;
        uint32_t adjusted_dest = EXT_STORAGE_ADDR_NEW_BASE + num_pages;
        NRF_LOG_INFO("adjusted_dest=%p", adjusted_dest);

        uint32_t remaining_space = SPI_FLASH_PAGE_LEN - buffer_offset;
        uint32_t bytes_to_copy = (len < remaining_space) ? len : remaining_space;
        uint32_t bytes_leftover = (len > remaining_space) ? (len - bytes_to_copy) : 0;

        memcpy(buffer + buffer_offset, p_src, bytes_to_copy);
        buffer_offset += bytes_to_copy;
        p_src_ptr += bytes_to_copy;

        if (buffer_offset == SPI_FLASH_PAGE_LEN || remaining_space == bytes_to_copy)
        {
            //lint -save -e611 (Suspicious cast)
            rc = ext_fstorage_write(adjusted_dest, buffer, SPI_FLASH_PAGE_LEN, (void *)callback, false);
            //lint -restore

            NRF_LOG_INFO("ext_fstorage_write(addr=%p, src=%p, len=%d bytes), queue usage: %d",
                            adjusted_dest, buffer, SPI_FLASH_PAGE_LEN, m_flash_operations_pending);

            buffer_offset = 0; // Reset the buffer offset
        } // fill the page

        if (bytes_leftover > 0)
        {
            // reset the buffer and fill in any leftover
            memset(buffer, 0x00, SPI_FLASH_PAGE_LEN);
            memcpy(buffer, p_src_ptr, bytes_leftover);
            buffer_offset = 0;
            buffer_offset += bytes_leftover;
        }

        // always fake out
        //lint -save -e611 (Suspicious cast)
        rc = ext_fstorage_write(dest, p_src, len, (void *)callback, true);
        //lint -restore
    }
    else
    {
        //lint -save -e611 (Suspicious cast)
        rc = nrf_fstorage_write(&m_fs, dest, p_src, len, (void *)callback);
        //lint -restore
        NRF_LOG_INFO("nrf_fstorage_write(addr=%p, src=%p, len=%d bytes), queue usage: %d",
                        dest, p_src, len, m_flash_operations_pending);        
    }

    if ((NRF_LOG_ENABLED) && (rc == NRF_SUCCESS))
    {
        m_flash_operations_pending++;
    }
    else
    {
        NRF_LOG_WARNING("nrf_fstorage_write() failed with error 0x%x.", rc);
    }

    return rc;
}



ret_code_t nrf_dfu_flash_erase(uint32_t                 page_addr,
                               uint32_t                 num_pages,
                               nrf_dfu_flash_callback_t callback)
{
    ret_code_t rc;

    NRF_LOG_INFO("nrf_fstorage_erase(addr=0x%p, len=%d pages), queue usage: %d",
                  page_addr, num_pages, m_flash_operations_pending);

    if (EXT_STORAGE_IS_ADDR(page_addr))
    {
        uint32_t relative_byte_address = page_addr - EXT_STORAGE_ADDR_BASE;
        uint32_t pages = relative_byte_address / SPI_FLASH_PAGE_LEN;
        uint32_t adjusted_page_addr = EXT_STORAGE_ADDR_NEW_BASE + pages;

        if (adjusted_page_addr % SPI_FLASH_PAGES_IN_BLOCK == 0)
        {
            // ff4: input is in blocks
            //lint -save -e611 (Suspicious cast)
            rc = ext_fstorage_erase(adjusted_page_addr, 1, (void *)callback, false);
            //lint -restore
        }
        //lint -save -e611 (Suspicious cast)
        rc = ext_fstorage_erase(page_addr, num_pages, (void *)callback, true);
        //lint -restore        
    }
    else
    {
        //lint -save -e611 (Suspicious cast)
        rc = nrf_fstorage_erase(&m_fs, page_addr, num_pages, (void *)callback);
        //lint -restore
    }

    if ((NRF_LOG_ENABLED) && (rc == NRF_SUCCESS))
    {
        m_flash_operations_pending++;
    }
    else
    {
        NRF_LOG_WARNING("nrf_fstorage_erase() failed with error 0x%x.", rc);
    }

    return rc;
}
