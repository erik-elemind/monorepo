/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: June, 2021
 * Author:  Paul Adelsbach
 *
 * Description: External flash low level interface.
 * Implements the commands outlined in the flash datasheet.
 */

#pragma once

#include <stdint.h>
#include "sdk_errors.h" // for ret_code_t
#include "app_util.h" // for STATIC_ASSERT()

// Define the page size in bytes.
#define SPI_FLASH_PAGE_LEN      (2048)
// Define the block size in bytes. This is the smallest erasable chunk.
#define SPI_FLASH_PAGES_IN_BLOCK  (64)
#define SPI_FLASH_BLOCK_LEN    (SPI_FLASH_PAGES_IN_BLOCK * SPI_FLASH_PAGE_LEN)
#define SPI_FLASH_OTA_START_ADDR (260864)
#define SPI_FLASH_OTA_NUM_BLOCKS (20) //this should match with NXP fatfs file config 

// Define status register addresses
#define status_protection_reg_addr 0xA0
#define status_config_reg_addr 0xB0
#define status_only_reg_addr 0xC0

// Protection Register (Status Register 1) type
typedef union {
  struct {
    // lsb
    bool    srp1 : 1;
    bool    wpe  : 1;
    bool    tb   : 1;
    uint8_t bp   : 4;
    bool    srp0 : 1;
    // msb
  };
  uint8_t raw;
} nand_protection_reg_t;

// Configuration Register (Status Register 2) type
typedef union {
  struct {
    // lsb
    bool    hdis : 1;
    uint8_t ods  : 2;
    bool    buf  : 1;
    uint8_t ecce : 4;
    bool    sr1l : 1;
    bool    otpe : 1;
	bool    otpl : 1;
    // msb
  };
  uint8_t raw;
} nand_configuration_reg_t;

// Status Register (Status Register 3) type
typedef union {
  struct {
    // lsb
    bool    busy     : 1;
    bool    wel      : 1;
    bool    efail    : 1;
    bool    pfail    : 1;
    uint8_t ecc      : 2;
    uint8_t reserved : 2;
    // msb
  };
  uint8_t raw;
} nand_status_reg_t;

// The following timeouts can be used to scale timeouts for different operations.
// These are specifically for high performance mode of the chip.
#define SPI_FLASH_TIMEOUT_PAGE_PROG         50      // tPP: typical=0.85ms, max=4ms
#define SPI_FLASH_TIMEOUT_SECTOR_ERASE      300     // tSE: typical=40ms, max=240ms
#define SPI_FLASH_TIMEOUT_CHIP_ERASE        160000  // tCE: typical=50s, max=150s

// Define the bitfields of the Status Register in the flash chip.
typedef union 
{
    struct 
    {
        uint8_t write_in_prog   :1;
        uint8_t write_enable    :1;
        uint8_t block_prot      :4;
        uint8_t quad_enable     :1;
        uint8_t sr_write_enable :1;
    };
    uint8_t byte;
} status_reg_t;
STATIC_ASSERT(1 == sizeof(status_reg_t));

/**@brief Read device ID from flash
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_read_id(void);


/**@brief Read page into cache
 * 
 * Upon success, the read data is available in the buffer.
 * 
 * @param[in]       addr    Flash address to read from
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_read_page(const uint32_t addr);

/**@brief Read flash memory
 * 
 * Upon success, the read data is available in the buffer.
 * 
 * @param[in]       addr    Flash address to read from
 * @param[inout]    len     Number of bytes to read. 
 *                          Updated upon success to reflect the actual length.
 * @param[in]       data    Buffer for read data
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_read_mem(const uint32_t addr, uint32_t* len, uint8_t* data);

/**@brief Sets the write enable bit as needed for write and erase commands.
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_write_enable(void);

/**@brief Read the status register.
 * 
 * @param[in]       addr    Address of status register
 * 
 * @param[out]      status  Contains the status bits read from the register.
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_status_read(nand_status_reg_t* status, uint8_t addr);

/**@brief Send the chip erase command.
 * 
 * Upon success, the erase procedure is initiated. Caller should read the 
 * status register to confirm completion before issuing another command.
 * 
 * This takes a long time and is not practical to use in production.
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_chip_erase(void);

/**@brief Send the block erase command.
 * 
 * Upon success, the erase procedure is initiated. Caller should read the 
 * status register to confirm completion before issuing another command.
 * 
 * @param[in]       addr    Flash block address to erase. 
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_block_erase(uint32_t addr);

/**@brief Load program (write) command.
 * 
 * Upon success, the write procedure is initiated. Caller should read the 
 * status register to confirm completion before issuing another command.
 * Caller need not preserve the data buffer following this call.
 * 
 * @param[in]       addr    Flash address to load data (eventually)
 * @param[inout]    len     Number of bytes to write.
 *                          Updated upon success to reflect the actual number 
 *                          of bytes.
 * @param[in]       data    Buffer containing data to load.
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_load_program(uint32_t addr, uint32_t* len, const uint8_t* data);

/**@brief Send a page program (write) command.
 * 
 * Upon success, the write procedure is initiated. Caller should read the 
 * status register to confirm completion before issuing another command.
 * Caller need not preserve the data buffer following this call.
 * 
 * @param[in]       addr    Flash address to write to
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_cmd_page_program(uint32_t addr);

/**@brief Initialize the low level flash interface.
 *
 * @retval  NRF_SUCCESS, if the operation was successful. Otherwise, an error
 *          code is returned.
 */
ret_code_t ext_flash_init(void);

