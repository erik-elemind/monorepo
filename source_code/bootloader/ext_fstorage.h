/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: June, 2021
 * Author:  Paul Adelsbach
 *
 * Description: External flash storage interface.
 * API is modeled after nrf_fstorage.h.
 * Calls are blocking for simplicity and code size.
 */

#pragma once

// Reuse the fstorage callback for compatibility with fstorage.
#include "nrf_fstorage.h"

// Define the virtual start address of external flash
// Bits [23:0] are assumed to be 0.
#define EXT_STORAGE_ADDR_BASE       (0xC0000000)
#define EXT_STORAGE_ADDR_NEW_BASE   (0x0003FB00)
// Size of external flash, in bytes
#define EXT_STORAGE_SIZE (SPI_FLASH_OTA_NUM_BLOCKS*SPI_FLASH_BLOCK_LEN)
// Returns true if address is in external flash range
#define EXT_STORAGE_IS_ADDR(addr)   ((addr) >= EXT_STORAGE_ADDR_BASE && \
                                     (addr) < (EXT_STORAGE_ADDR_BASE + EXT_STORAGE_SIZE))

/**@brief   Function for initializing fstorage.
 *
 * @param[in]   evt_handler     Callback handler.
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_fstorage_init(nrf_fstorage_evt_handler_t evt_handler);

/**@brief   Function for reading data from flash.
 *
 * Copy @p len bytes from @p addr to @p p_dest.
 *
 * @param[in]   addr    Address in flash where to read from.
 * @param[in]   p_dest  Buffer where the data should be copied.
 * @param[in]   len     Length of the data to be copied (in bytes).
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_fstorage_read(uint32_t               addr,
                             void                 * p_dest,
                             uint32_t               len);

/**@brief   Function for writing data to flash.
 *
 * Write @p len bytes from @p p_src to @p dest.
 *
 * @param[in]   dest        Address in flash memory where to write the data.
 * @param[in]   p_src       Data to be written.
 * @param[in]   len         Length of the data (in bytes).
 * @param[in]   p_param     User-defined parameter passed to the event handler (may be NULL).
 * @param[in]   notify      Bool to notify event handler
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_fstorage_write(uint32_t               dest,
                              void           const * p_src,
                              uint32_t               len,
                              void                 * p_param,
                              bool                   notify);

/**@brief   Function for erasing flash pages.
 *
 * @details This function erases @p len pages starting from the page at address @p page_addr.
 *          The erase operation must be initiated on a page boundary.
 *
 * @param[in]   page_addr   Address of the page to erase.
 * @param[in]   num_pages   Number of pages to erase.
 * @param[in]   p_param     User-defined parameter passed to the event handler (may be NULL).
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_fstorage_erase(uint32_t               page_addr,
                              uint32_t               num_pages,
                              void                 * p_param);

/**@brief   Calculate the number of pages to erase based on address and length.
 *
 * @param[in]   addr        Address of the page to erase.
 * @param[in]   len         Number of bytes in the buffer.
 *
 * @retval  Number of pages is returned.
 */
uint32_t ext_fstorage_page_count(uint32_t addr,
                                 uint32_t len);

/**@brief   Calculate the number of blocks to erase based on address and length.
 *
 * @param[in]   addr        Page address of the block to erase.
 * @param[in]   len         Number of bytes in the buffer.
 *
 * @retval  Number of blocks is returned.
 */
uint32_t ext_fstorage_block_count(uint32_t page_addr,
                                 uint32_t len);