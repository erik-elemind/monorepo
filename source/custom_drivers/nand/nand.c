/*
 * spi_nand.c
 *
 * Copyright (C) 2020 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Generic NAND flash driver.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

#include "nand.h"
#include "nand_platform.h"

// Set the log level for this file
#define LOG_LEVEL_MODULE  LOG_WARN
#include "loglevels.h"

/// Logging prefix
static const char* TAG = "nand";


// Check for a define with the number of ECC bits in the status register
#ifndef NAND_ECC_STATUS_REG_BIT_COUNT
#error "Must define NAND_ECC_STATUS_REG_BIT_COUNT"
#endif

/** Convert 24-bit page address (in a uint32_t) to 3-byte MSB-first array.

    Don't mask off the first byte--the uint8_t conversion will do that
    for us. This format allows us to use the uint32 address to directly
    initialize the address field in a xxx_command_t struct, i.e.:

      .page_addr = PAGE_ADDR_TO_MSB_FIRST_ARRAY(page_addr)
 */
#define PAGE_ADDR_TO_MSB_FIRST_ARRAY(x) \
  { (uint8_t)(x >> 16), (uint8_t)(x >> 8), (uint8_t)x }

/** Convert 16-bit page offset (in a uint16_t) to 2-byte MSB-first array.

    See notes above. Page offset is the address within a page (since
    pages are 4K+256 bytes, max page offset is 0x10FF == 4352).
 */
#define PAGE_OFFSET_TO_MSB_FIRST_ARRAY(x) \
  { (uint8_t)(x >> 8), (uint8_t)x }


// Flash commands (originally used with GigaDevice GD5F family)
typedef enum {
  COMMAND_WRITE_ENABLE = 0x06,
  COMMAND_WRITE_DISABLE = 0x04,
  COMMAND_GET_FEATURES = 0x0F,
  COMMAND_SET_FEATURE = 0x1F,
  COMMAND_PAGE_READ = 0x13,
  COMMAND_READ_FROM_CACHE = 0x03,
  COMMAND_FAST_READ_FROM_CACHE = 0x0B,

  COMMAND_READ_FROM_CACHE_X_2 = 0x3B,
  COMMAND_READ_FROM_CACHE_X_4 = 0x6B,
  COMMAND_READ_FROM_CACHE_DUAL_IO = 0xBB,
  COMMAND_READ_FROM_CACHE_QUAD_IO = 0xEB,

  COMMAND_READ_ID = 0x9F,
  COMMAND_PROGRAM_LOAD = 0x02,

  COMMAND_PROGRAM_LOAD_X_4 = 0x32,

  COMMAND_PROGRAM_EXECUTE = 0x10,
  COMMAND_PROGRAM_LOAD_RANDOM = 0x84,
  COMMAND_BLOCK_ERASE = 0xD8,
  COMMAND_RESET = 0xFF
} nand_command_t;

// Device constants

// Prevent padding bytes in register union structs. (This is popped back below.)
#pragma pack(push,1)

// Status register type
typedef union {
  struct {
    // lsb
    bool oip : 1;
    bool wel : 1;
    bool e_fail : 1;
    bool p_fail : 1;
    uint8_t eccs : NAND_ECC_STATUS_REG_BIT_COUNT; // (this bit count varies between chips)
    bool reserved : (8-4-NAND_ECC_STATUS_REG_BIT_COUNT);
    // msb
  };
  uint8_t raw;
} nand_status_reg_t;

// Protection register type
typedef union {
  struct {
    // lsb
    bool reserved1 : 1;
    bool cmp : 1;
    bool inv : 1;
    bool bp0 : 1;
    bool bp1 : 1;
    bool bp2 : 1;
    bool reserved2: 1;
    bool brwd : 1;
    // msb
  };
  uint8_t raw;
} nand_protection_reg_t;

// Feature1 register type
typedef union {
  struct {
    // lsb
    bool qe : 1;
    uint8_t reserved : 3;
    bool ecc_en : 1;
    bool reserved2 : 1;
    bool otp_en: 1;
    bool otp_prt : 1;
    // msb
  };
  uint8_t raw;
} nand_feature1_reg_t;

// Feature2 register type
typedef union {
  struct {
    // lsb
    uint8_t reserved1 : 5;
    uint8_t ds_io : 2;
    bool reserved2 : 1;
    // msb
  };
  uint8_t raw;
} nand_feature2_reg_t;

// DS (drive strength) values from feature2_reg_t
enum {
  DS_100_PERCENT = 0,
  DS_75_PERCENT = 1,
  DS_50_PERCENT = 2,
  DS_25_PERCENT = 3,
};

// Get Features command
typedef union {
  struct __attribute__((packed)) {
    uint8_t command; ///< COMMAND_GET_FEATURES
    uint8_t reg; ///< From feature_reg_t
  };
  uint8_t raw[2];
} get_features_command_t;

// Set Feature command
typedef union {
  struct __attribute__((packed)) {
    uint8_t command; ///< COMMAND_SET_FEATURE
    uint8_t reg; ///< From feature_reg_t
    uint8_t data; ///< protection_reg_t, feature_1_reg_t, or feature_2_reg_t
  };
  uint8_t raw[3];
} set_feature_command_t;

// Page Read to Cache command
typedef union {
  struct __attribute__((packed)) {
    uint8_t command; ///< COMMAND_PAGE_READ
    uint8_t page_addr[3]; ///< Select page to read (24-bit address), MSB-first
  };
  uint8_t raw[4];
} page_read_command_t;

// Read From Cache command
typedef union {
  struct __attribute__((packed)) {
    uint8_t command; ///< COMMAND_READ_FROM_CACHE
    uint8_t dummy;
    uint8_t addr[2]; ///< Address to start read (within page), MSB-first
  };
  uint8_t raw[4];
} read_from_cache_command_t;

// Read ID command response
typedef union {
  struct __attribute__((packed)) {
    uint8_t mfg_id;
    uint8_t device_id[2];
  };
  uint8_t raw[3];
} read_id_response_t;

// Program Load command
typedef union {
  struct __attribute__((packed)) {
    uint8_t command; ///< COMMAND_PROGRAM_LOAD
    uint8_t addr[2]; ///< Address to start write (within page), MSB-first
  };
  uint8_t raw[3];
} program_load_command_t;

// Program Execute command
typedef union {
  struct __attribute__((packed)) {
    uint8_t command; ///< COMMAND_PROGRAM_EXECUTE
    uint8_t page_addr[3]; ///< Page to write (24-bit address), MSB-first
  };
  uint8_t raw[4];
} program_execute_command_t;

// Erase Block command
typedef union {
  struct __attribute__((packed)) {
    uint8_t command; ///< COMMAND_BLOCK_ERASE
    uint8_t page_addr[3]; ///< Page to write (24-bit address), MSB-first
  };
  uint8_t raw[4];
} erase_block_command_t;

#pragma pack(pop)


/** Send command (no response data).

    Sends provided command.

    @param user_data Flash SPI handle
    @param command Command to send
    @param command_len Command length, in bytes

    @return 0 if command and response completed
    succesfully, or error code
 */
static int
nand_command(
  nand_user_data_t *user_data,
  uint8_t* p_command,
  uint8_t command_len
  )
{
  return nand_platform_command_response(p_command, command_len, NULL, 0);
}

/** Set feature register.

    @param user_data Flash SPI handle
    @param feature_reg Feature register to read
    @param data Data to write (1 byte)

    @return 0 if command and response completed
    succesfully, or error code
 */
static int
nand_set_feature_reg(
  nand_user_data_t *user_data,
  feature_reg_t feature_reg,
  uint8_t data
  )
{
  LOGV(TAG, "nand_get_feature_reg: feature_reg 0x%X (1 B) in: 0x%X (1 B)", feature_reg, data);

  set_feature_command_t command = {
    .command = COMMAND_SET_FEATURE,
    .reg = feature_reg,
    .data = data
  };
  int status = nand_command(user_data,
    command.raw, sizeof(command.raw));

  return status;
}

/** Thin convenience function for readability.

    Output: nand_status_reg_t *status_reg so we can check for
            OIP, E_FAIL, P_FAIL, and ECC bits

    @return 0 or error
 */
static inline int
nand_get_status_reg(
  nand_user_data_t *user_data,
  nand_status_reg_t *status_reg
  )
{
  LOGV(TAG, "nand_get_status_reg: status_reg 0x%X (1 B)", FEATURE_REG_STATUS);

  // Get status register
  int status = nand_get_feature_reg(user_data,
    FEATURE_REG_STATUS, &(status_reg->raw));

  LOGV(TAG, "...nand_get_status_reg: out: 0x%X (1 B)", (uint8_t)status_reg->raw);

  return status;
}

/** Evaluate the status register ECC bits for data safety analysis. */
int
nand_ecc_result(uint8_t status_register_byte)
{
  int result;
  uint32_t ecc_bits_corrected = ecc_bits_map(status_register_byte);
//  printf("nand_ecc_result: ecc_bits_corrected: %lu\n", ecc_bits_corrected);

  if (ecc_bits_corrected == 0) {
    result = NAND_NO_ERR;  // Clean data :)
  }
  else if (ecc_bits_corrected <= NAND_ECC_SAFE_CORRECTED_BIT_COUNT) {
    result = NAND_NO_ERR;  // Fib to minimize bad block flagging
  }
  else if (ecc_bits_corrected <= NAND_ECC_MAX_CORRECTED_BIT_COUNT) {
    result = NAND_ECC_OK; // The end draws nigh...
  }
  else {
    result = NAND_ECC_FAIL;      // Bad data :(
  }

  return result;
}

/** Read and evaluate the status register ECC bits for data safety analysis. */
int nand_ecc_bits(nand_user_data_t *user_data, uint32_t* num_bits)
{
  nand_status_reg_t status_reg;
  // Read the status register
  int status = nand_get_status_reg(user_data, &status_reg);
  if (status == 0) {
    // Decode the status register, and return
    // the actual number of ECC corrected bits.
    *num_bits = ecc_bits_map(status_reg.raw);
  }
  return status;
}


/** Get NAND ID. */
int
nand_get_id(
  nand_user_data_t *user_data,
  uint8_t* p_mfg_id,
  uint16_t* p_device_id
  )
{
  LOGV(TAG, "nand_get_id");

  uint8_t command = COMMAND_READ_ID;
  read_id_response_t response;
  int status = nand_platform_command_response(
    &command, sizeof(command),
    (uint8_t*)&response, sizeof(response));

  *p_mfg_id = response.mfg_id;
  *p_device_id = (response.device_id[0] << 8) | (response.device_id[1]);

  return status;
}

/** Get feature register.

    @param user_data Flash SPI handle
    @param feature_reg Feature register to read
    @param[out] data Data received (1 byte)

    @return 0 if command and response completed
    succesfully, or error code
 */
int
nand_get_feature_reg(
  nand_user_data_t *user_data,
  feature_reg_t feature_reg,
  uint8_t* p_data
  )
{
  LOGV(TAG, "nand_get_feature_reg: feature_reg 0x%X (1 B)", feature_reg);

  get_features_command_t command = {
    .command = COMMAND_GET_FEATURES,
    .reg = feature_reg
  };
  int status = nand_platform_command_response(
    command.raw, sizeof(command.raw), p_data, 1);

  LOGV(TAG, "...nand_get_feature_reg: out: 0x%X (1 B)", *(uint8_t *)p_data);

  return status;
}

/** Unlock flash. */
int
nand_unlock(nand_user_data_t *user_data)
{
  LOGV(TAG, "nand_unlock");

  nand_protection_reg_t protection_reg;
  protection_reg.cmp = 0;
  protection_reg.inv = 0;
  protection_reg.bp0 = 0;
  protection_reg.bp1 = 0;
  protection_reg.bp2 = 0;
  protection_reg.brwd = 0;

  int status = nand_set_feature_reg(user_data,
    FEATURE_REG_PROTECTION, protection_reg.raw);

  return status;
}

/** Enable/disable on-chip ECC. */
int
nand_ecc_enable(nand_user_data_t *user_data, bool enable)
{
  LOGV(TAG, "nand_ecc_enable: %d", (int)enable);

  nand_feature1_reg_t feature1_reg;
  int status = nand_get_feature_reg(user_data,
    FEATURE_REG_FEATURE1, &feature1_reg.raw);
  if (status < 0) {
    return status;
  }

  feature1_reg.ecc_en = enable;
  status = nand_set_feature_reg(user_data,
    FEATURE_REG_FEATURE1, feature1_reg.raw);

  return status;
}

/** Read flash page. */
int
nand_read_page(
  nand_user_data_t *user_data,
  uint32_t block,
  uint32_t page,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  int status;
  int ecc_status;

  // page_addr here is the bitwise combo of both block number and page number.
  uint32_t page_addr = nand_block_page_to_page_addr(block, page, user_data->chipinfo->block_addr_offset);

  LOGV(TAG, "nand_read_page: blk %d, pg %d, pg_addr 0x%lX, pg_offset %d, p_data %p, len %d",
    (int)block, (int)page, page_addr, (int)page_offset, p_data, (int)data_len);

  status = nand_read_page_into_cache(user_data, page_addr);
  LOGV(TAG, "nand_read_page_into_cache: status: %d", (int) status);
  // If ECC_FAIL, still try to read out the page data below (despite ECC errors).
  if (status < 0 && status != NAND_ECC_FAIL) {
    LOGE(TAG, "nand_read_page(%lu.%lu): nand_read_page_into_cache: %d", block, page, (int)status);
    return status;
  }

  // status could be: NO_ERR (0), ECC_OK (1), ECC_FAIL (-2)
  ecc_status = status;

  // Read page out from the chip's internal cache
  // This way we should get most data, even if it includes some corrupt bits
  status = nand_read_page_from_cache(user_data, page_offset, p_data, data_len);
  LOGV(TAG, "nand_read_page_from_cache: status: %d", (int) status);
  if (status < 0) {
    LOGE(TAG, "nand_read_page(%lu.%lu): nand_read_page_from_cache: %d", block, page, (int)status);
    return status;
  }

  if (ecc_status == NAND_ECC_FAIL) {
    LOGW(TAG, "nand_read_page(%lu.%lu): status_reg.eccs: %d, returning NAND_ECC_FAIL", block, page, ecc_status);
    return NAND_ECC_FAIL;
  }

  if (ecc_status == NAND_ECC_OK) {
    status = NAND_ECC_OK;
  }

  // return NAND_NO_ERR or NAND_ECC_OK
  return status;
}

/** Write flash page. */
int
nand_write_page(
  nand_user_data_t *user_data,
  uint32_t block,
  uint32_t page,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  int status;

  // page_addr here is the bitwise combo of both block number and page number.
  uint32_t page_addr = nand_block_page_to_page_addr(block, page, user_data->chipinfo->block_addr_offset);

  LOGV(TAG, "nand_write_page: page_addr 0x%lX, page_offset %d, p_data %p, data_len %d",
    page_addr, (int)page_offset, p_data, (int)data_len);

  ///@todo BH - Check page_addr, page_offset, data_len value

  // Write data to program cache
  status = nand_write_into_page_cache(user_data, page_offset, p_data, data_len);
  LOGV(TAG, "nand_write_into_page_cache: status: %d", (int) status);
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): COMMAND_PROGRAM_LOAD error: %d",
      status);
    return status;
  }

  status = nand_program_page_cache(user_data, page_addr);
  LOGV(TAG, "nand_program_page_cache: status: %d", (int) status);
  return status;
}

/** Copy page: Copy a page into another address using internal cache.

    @param user_data Platform/user data handle
    @param src_page_addr Source 24-bit page address (incl. block shifted over some bits)
    @param dest_page_addr Destination 24-bit page address (incl. block shifted over some bits)

    NOTE: Unlike other functions, this does not take a block index, page index pair.
    It takes the page address, as returned from nand_block_page_to_page_addr().

    @return 0 if successful, or other value if error
*/
// IMPORTANT: This function will not work for our part on arbitrary page
// pairs -- only some cases are permissible. See comments in
// dhara_nand.c for details.
int
nand_copy_page_from_cache(
  nand_user_data_t *user_data,
  uint32_t src_page_addr,
  uint32_t dest_page_addr
  )
{
  LOGV(TAG, "nand_copy_page_from_cache: src_pg_addr 0x%lX, dest_pg_addr 0x%lX", src_page_addr, dest_page_addr);

  int status;
  int ecc_status;

  status = nand_read_page_into_cache(user_data, src_page_addr);
  // If ECC_FAIL, still try to read out the page data below (despite ECC errors).
  if (status < 0 && status != NAND_ECC_FAIL) {
    LOGE(TAG, "nand_copy_page_from_cache(0x%lx->0x%lx): nand_read_page_into_cache: %d", src_page_addr, dest_page_addr, (int)status);
    return status;
  }

  // Check ECCS bits after read from cache operation. Note that ECC failures
  // are expected normal behavior as the chip wears out.
  // status could be: NO_ERR (0), ECC_OK (1), ECC_FAIL (-2)
  ecc_status = status;

  if (ecc_status == NAND_ECC_FAIL) {
    LOGW(TAG, "nand_copy_page_from_cache(0x%lx->0x%lx): status_reg.eccs: %d, returning NAND_ECC_FAIL", src_page_addr, dest_page_addr, ecc_status);
    return NAND_ECC_FAIL;
  }

  if (ecc_status == NAND_ECC_OK) {
    status = NAND_ECC_OK;
  }

  status = nand_program_page_cache(user_data, dest_page_addr);
  if (status < 0) {
    LOGE(TAG, "nand_copy_page_from_cache(0x%lx->0x%lx): nand_read_page_from_cache: %d", src_page_addr, dest_page_addr, (int)status);
    return status;
  }

  // return NAND_NO_ERR or NAND_ECC_OK
  return status;
}

/** Read flash page cache. */
int
nand_read_page_from_cache(
  nand_user_data_t *user_data,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  LOGV(TAG, "nand_read_page_from_cache: page_offset %d, p_data %p (%d B)",
    (int)page_offset, p_data, (int)data_len);

  int status;

  read_from_cache_command_t read_from_cache = {
    .command = COMMAND_READ_FROM_CACHE,
    .addr = PAGE_OFFSET_TO_MSB_FIRST_ARRAY(page_offset)
  };
  status = nand_platform_command_response(
    read_from_cache.raw, sizeof(read_from_cache.raw), p_data, data_len);

  return status;
}

/** Load flash page into the NAND chip's onboard cache, return ECC result. 
    Returns: <0, or One of NO_ERR (0), ECC_OK (1), or ECC_FAIL (-2)
*/
int
nand_read_page_into_cache(
  nand_user_data_t *user_data,
  uint32_t page_addr
  )
{
  int status;
  nand_status_reg_t status_reg;

#if 0
  if (g_runtime_log_level == LOG_VERBOSE) {
    nand_print_detailed_status(user_data);
  }
#endif
  ///@todo BH - Check page_addr, page_offset, data_len value

  // Start page read into flash internal cache
  page_read_command_t page_read = {
    .command = COMMAND_PAGE_READ,
    .page_addr = PAGE_ADDR_TO_MSB_FIRST_ARRAY(page_addr)
  };
  status = nand_command(user_data, page_read.raw, sizeof(page_read.raw));
  if (status < 0) {
    LOGE(TAG, "nand_read_page_into_cache(): COMMAND_PAGE_READ error: %d", status);
    return status;
  }

  // Wait for internal cache read to complete
  while(true) {
    // Check OIP (Operation In Progress) bit
    status = nand_get_status_reg(user_data, &status_reg);
    if(status == kStatus_Success){
      if(status_reg.oip == 0){
        break;
      }
    }
    // We are still waiting for the flash chip to be ready.
    nand_platform_yield_delay(0);   // 0 means just yield; this should take <1 ms (80 usecs)
  };
  if (status < 0) {
    LOGE(TAG, "nand_read_page_into_cache(): _is_device_ready() error: %d", status);
    return status;
  }

  // Check ECCS bits after read from cache operation. Note that ECC failures
  // are expected normal behavior as the chip wears out.
  return nand_ecc_result((uint8_t)status_reg.raw);  
}

/** Write flash page cache. */
int
nand_write_into_page_cache(
  nand_user_data_t *user_data,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  LOGV(TAG, "nand_write_into_page_cache: page_offset %d, p_data: 0x%lX... (%d B)",
    (int)page_offset, *(uint32_t *)p_data, (int)data_len);

  int status;

  // COMMAND_PROGRAM_LOAD: "SPI-NAND controller outputs 0xFF data to the NAND
  // for the address that data was not loaded by Program Load (02h) command."
  //
  // COMMAND_PROGRAM_LOAD_RANDOM: "When Program Execute (10h) command was issued
  // just after Program Load Random Data (84h) command, SPI-NAND controller
  // outputs contents of Cache Register to the NAND."
  program_load_command_t program_load = {
//      .command = COMMAND_PROGRAM_LOAD_RANDOM,
    .command = COMMAND_PROGRAM_LOAD,
    .addr = PAGE_OFFSET_TO_MSB_FIRST_ARRAY(page_offset)
  };
  status = nand_platform_command_with_data(
    program_load.raw, sizeof(program_load.raw), p_data, data_len);

  return status;
}

// Flush the NAND Chip internal buffer to the give 24-bit page_addr
int nand_program_page_cache(
  nand_user_data_t *user_data,
  uint32_t page_addr
  )
{
  int status;
  nand_status_reg_t status_reg;

  // Set write enable latch
  uint8_t write_enable_command = COMMAND_WRITE_ENABLE;
  status = nand_command(user_data,
    &write_enable_command, sizeof(write_enable_command));
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): COMMAND_WRITE_ENABLE error: %d",
      status);
    return status;
  }

  // Program data from cache into flash page
  program_execute_command_t program_execute = {
    .command = COMMAND_PROGRAM_EXECUTE,
    .page_addr = PAGE_ADDR_TO_MSB_FIRST_ARRAY(page_addr)
  };
  status = nand_command(user_data,
    program_execute.raw, sizeof(program_execute.raw));
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): COMMAND_PROGRAM_EXECUTE error: %d",
      status);
    return status;
  }

  // Wait for program execution to complete
  while(true) {
    // Check OIP (Operation In Progress) bit
    status = nand_get_status_reg(user_data, &status_reg);
    if(status == kStatus_Success){
      if(status_reg.oip == 0){
        break;
      }
    }
    // TODO: Handle other bad status
    // We are still waiting for the flash chip to be ready.
    nand_platform_yield_delay(0);   // 0 means just yield; this should take <1 ms (80 usecs)
  };
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): _is_device_ready() error: %d", status);
    return status;
  }

  // Check status register bit
  if (status_reg.p_fail == 1) {
    status = NAND_UNKNOWN_ERR;  // Program Failed
  }

  return status;
}

/** Check flash block. */
int
nand_block_status(
  nand_user_data_t *user_data,
  uint32_t page_addr,
  bool* p_good
  )
{
  ///@todo BH Check page_addr valid
  // NOTE: Only page 0 is valid for a factory bad block check

  int status;

  // check if ECC is enabled
  // according to the documentation, ecc should be disabled when checking the bad page mark.
  nand_feature1_reg_t feature1_reg;
  status = nand_get_feature_reg(user_data,
    FEATURE_REG_FEATURE1, &feature1_reg.raw);
  if (status != 0) {
    return status;
  }
  bool ecc_enabled = feature1_reg.ecc_en;

  // disable ECC
  if(ecc_enabled){
    feature1_reg.ecc_en = false;
    status = nand_set_feature_reg(user_data,
      FEATURE_REG_FEATURE1, feature1_reg.raw);
    if (status != 0) {
      return status;
    }
  }

  // Get block and page numbers
  uint32_t block, page;
  nand_page_addr_to_block_page(
    page_addr, user_data->chipinfo->block_addr_offset, &block, &page);
  // But always use page 0 since that is where the marker is.
  page = 0;

  // First byte of spare is the marker. Datasheet calls this the 
  // "Bad Block Mark".
  static const uint32_t GOOD_BLOCK_MARKER_ADDR = NAND_PAGE_SIZE;
  // Anything other than 0xFF is bad.
  static const uint8_t GOOD_BLOCK_MARKER = 0xFF;

  uint8_t data;
  status = nand_read_page(user_data,
    block, page, GOOD_BLOCK_MARKER_ADDR, &data, sizeof(data));
  if (status == 0) {
    *p_good = (data == GOOD_BLOCK_MARKER);
  }else{
    *p_good = false;
  }

  // enable ECC
  if(ecc_enabled){
    feature1_reg.ecc_en = true;
    status = nand_set_feature_reg(user_data,
      FEATURE_REG_FEATURE1, feature1_reg.raw);
    if (status != 0) {
      return status;
    }
  }

  return status;
}


/** Erase flash block. */
int
nand_erase_block(
  nand_user_data_t *user_data,
  uint32_t page_addr
  )
{
  int status;
  nand_status_reg_t status_reg;

  ///@todo BH - Check page_addr valid
  uint32_t block;
  uint32_t page;
  nand_page_addr_to_block_page(page_addr,user_data->chipinfo->block_addr_offset, &block, &page);
  LOGV(TAG, "nand_erase_block(): page_addr 0x%lx (%lu), page %lu of block %lu", page_addr, page_addr, page, block);

  // Set write enable latch
  uint8_t write_enable_command = COMMAND_WRITE_ENABLE;
  status = nand_command(user_data,
    &write_enable_command, sizeof(write_enable_command));
  if (status < 0) {
    LOGE(TAG, "nand_erase_block(): COMMAND_WRITE_ENABLE error: %d",
      status);
    return status;
  }

  // Erase block
  erase_block_command_t erase_block = {
    .command = COMMAND_BLOCK_ERASE,
    .page_addr = PAGE_ADDR_TO_MSB_FIRST_ARRAY((block << NAND_PAGES_PER_BLOCK_LOG2))
  };
  status = nand_command(user_data,
    erase_block.raw, sizeof(erase_block.raw));
  if (status < 0) {
    LOGE(TAG, "nand_erase_block(): COMMAND_ERASE_BLOCK error: %d",
      status);
    return status;
  }

  /* Wait for erase block to complete (could take up to 5 ms--wait in
     between checks). */

  // "Typical: 3 ms, max: 5 ms". Minimize SPI bus traffic by sleeping the first 2 ms.
  nand_platform_yield_delay(2);

  do {
    // Check OIP (Operation In Progress) bit
    status = nand_get_status_reg(user_data, &status_reg);	
    if (status == NAND_NO_ERR && status_reg.oip == 1) {
      // We are still waiting for the flash chip to be ready.
      nand_platform_yield_delay(1);
    }
  } while (status == NAND_NO_ERR && status_reg.oip == 1);

  if (status < 0) {
    LOGE(TAG, "nand_erase_block(): _get_status_reg() error: %d", status);
    return status;
  }

  // Check E_FAIL bit in status reg
  if (status_reg.e_fail == 1) {
    LOGW(TAG, "nand_erase_block(): e_fail==1 on block: %ld, returning NAND_UNKNOWN_ERR", block);
    status = NAND_UNKNOWN_ERR;  // Erase Failed.
  }

  return status;
}

/* Output detailed flash status. */
int
nand_print_detailed_status(
  nand_user_data_t *user_data
  )
{
  int status;

  // Get protection register
  nand_protection_reg_t protection_reg;
  status = nand_get_feature_reg(user_data,
    FEATURE_REG_PROTECTION, &protection_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  Protection register (%02x): 0x%x\n",
      FEATURE_REG_PROTECTION, protection_reg.raw);
    printf("    cmp: %d\n", protection_reg.cmp);
    printf("    inv: %d\n", protection_reg.inv);
    printf("    bp0: %d\n", protection_reg.bp0);
    printf("    bp1: %d\n", protection_reg.bp1);
    printf("    bp2: %d\n", protection_reg.bp2);
    printf("    brwd: %d\n", protection_reg.brwd);
  }
  else {
    printf("Error reading protection register: %d\n", status);
  }

  // Get feature1 register
  nand_feature1_reg_t feature1_reg;
  status = nand_get_feature_reg(user_data,
    FEATURE_REG_FEATURE1, &feature1_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  Feature1 register (%02x): 0x%x\n",
      FEATURE_REG_FEATURE1, feature1_reg.raw);
    printf("    qe: %d\n", feature1_reg.qe);
    printf("    ecc_en: %d\n", feature1_reg.ecc_en);
    printf("    otp_en: %d\n", feature1_reg.otp_en);
    printf("    otp_prt: %d\n", feature1_reg.otp_prt);
  }
  else {
    printf("Error reading feature1 register: %d\n", status);
  }

  // Get status register
  nand_status_reg_t status_reg;
  status = nand_get_feature_reg(user_data,
    FEATURE_REG_STATUS, &status_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  Status register (%02x): 0x%x\n",
      FEATURE_REG_STATUS, status_reg.raw);
    printf("    oip: %d\n", status_reg.oip);
    printf("    wel: %d\n", status_reg.wel);
    printf("    e_fail: %d\n", status_reg.e_fail);
    printf("    p_fail: %d\n", status_reg.p_fail);
    printf("    eccs: %d\n", status_reg.eccs);
  }
  else {
    printf("Error reading status register: %d\n", status);
  }

  // Get feature2 register
  nand_feature2_reg_t feature2_reg;
  status = nand_get_feature_reg(user_data,
    FEATURE_REG_FEATURE2, &feature2_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  Feature2 register (%02x): 0x%x\n",
      FEATURE_REG_FEATURE2, feature2_reg.raw);
    printf("    ds_io: %d\n", feature2_reg.ds_io);
  }
  else {
    printf("Error reading feature2 register: %d\n", status);
  }

  return status;
}

