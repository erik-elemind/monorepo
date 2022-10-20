/*
 * nand_W25N04KW.h
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Generic NAND flash driver.
*/
#ifndef NAND_H
#define NAND_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include "nand_platform.h"

#ifdef __cplusplus
extern "C" {
#endif


// These values depend on the chip's addressing. It's usually a 24-bit
// address but the number of bits for page vs. block can vary.
typedef struct {
  uint8_t block_addr_offset;
  size_t layout_size_bytes;
} nand_chipinfo_t;
// Caller to provide an implementation of this
extern const nand_chipinfo_t nand_chipinfo;

// Custom struct to hold all info for driver and address translations:
typedef struct nand_user_data {
  const nand_chipinfo_t* chipinfo;

  size_t layout_size_bytes;
  uint8_t *layout_buffer;
  uint32_t layout_page_addr;  // The address of the page currently in layout_buffer

} nand_user_data_t;

/*
  WARN: ECC Data Safety

  Most Flash chips use compatible register addresses and values.
  But in the status register, the number of eccs bits vs reserved bits varies 
  between chips. Also, the number of ECC errors which can be corrected by the
  chip varies. This is critically important for data safety.

  nand_ecc_result() returns NO_ERR so long as the number of corrected 
  bits is within a "safe" margin. Meaning, NAND_ECC_SAFE_CORRECTED_BIT_COUNT
  (or less) bits were found flipped but corrected by ECC. 

  In UFFS, if CONFIG_BAD_BLOCK_POLICY_STRICT is set, then ECC correcting more than 
  NAND_ECC_SAFE_CORRECTED_BIT_COUNT bits will move the (valid ECC-corrected) 
  data to a fresh block and mark this block as bad. Any other filesystem or 
  NAND code would need to do something similar.

  Some flash chips can only correct 1 bit flip. In that case a value of 
    NAND_ECC_SAFE_CORRECTED_BIT_COUNT == 0
  is recommended because it's only 1 bit flip away from losing data.
  Some flash chips can correct more (e.g. 8 bit flips). So 1 or 2 bit flips 
  should probably not trigger marking a block as bad.

  A paranoid approach is just setting NAND_ECC_SAFE_CORRECTED_BIT_COUNT to zero,
  but that will reduce the lifespan of the chip because blocks will be 
  marked bad more often (and data will need to be moved/copied over more often, 
  reducing performance).

  WARN: This must be < ECC_MAX_CORRECTED_BIT_COUNT to prevent impending data loss.
  NOTE: Set this == ECC_MAX_CORRECTED_BIT_COUNT to disable UFFS bad block management
  See also: uffs_config.h for UFFS options.
*/
#ifndef NAND_ECC_SAFE_CORRECTED_BIT_COUNT
  // Assume the chip supports 8 ECC bits, of which up to 4 ECC flips are safe:
  #define NAND_ECC_SAFE_CORRECTED_BIT_COUNT 6
#endif

// ECC Data Correction:
//
// How many bits can the NAND chip correct with ECC?
// This driver supports 8 ECC bits (modern chips) or 1 ECC bit (cheap/old chips).
// If not defined, we try to determine it based on the SAFE bit count above
// (either 8 bits or 1 bit).
//
#ifndef NAND_ECC_MAX_CORRECTED_BIT_COUNT
  #if NAND_ECC_SAFE_CORRECTED_BIT_COUNT == 0
    #define NAND_ECC_MAX_CORRECTED_BIT_COUNT 1
  #else
    #define NAND_ECC_MAX_CORRECTED_BIT_COUNT 8
  #endif
#endif


// For convenience, these functions return int results compatible with UFFS.
// These codes are not typedef'd because UFFS's callbacks also use raw ints.
// NOTE: Any hard errors are <0 in order to remain compatible with uffs_flash.h:
//
//   #define UFFS_FLASH_HAVE_ERR(e)		((e) < 0)   // (from uffs_flash.h)
//
#define NAND_NO_ERR		0		//!< no error
#define NAND_ECC_OK		1		//!< bit-flip found, but corrected by ECC (time to move the data!)
#define NAND_NOT_SEALED	2		//!< page spare area is not sealed properly (only for ReadPageWithLayout())
#define NAND_IO_ERR		-1		//!< I/O error
#define NAND_ECC_FAIL		-2		//!< ECC failed
#define NAND_BAD_BLK		-3		//!< bad block
#define NAND_CRC_ERR		-4		//!< CRC failed
#define NAND_UNKNOWN_ERR	-100	//!< unkown error?

// Feature registers
typedef enum {
  FEATURE_REG_PROTECTION = 0xA0,
  FEATURE_REG_CONFIGURATION = 0xB0,
  FEATURE_REG_STATUS = 0xC0,
  FEATURE_REG_ECC1 = 0x10,
  FEATURE_REG_ECC2 = 0x20,
  FEATURE_REG_ECC3 = 0x30,
  FEATURE_REG_ECC4 = 0x40,
  FEATURE_REG_ECC5 = 0x50,
} feature_reg_t;


/** Initialize the NAND flash and communications with the flash.

 */
int
nand_init();

/** Evaluate the status register ECC bits for data safety analysis.

  WARNING: Not all chips report the number of ECC bits in the status register.
  Further work is be necessary for chips which do not report 
  this status field, including the Macronix MX35LF1 family.

  @param status_register_byte  The NAND status register byte to evaluate.
  @return                      One of NO_ERR (0), ECC_OK (1), or ECC_FAIL (-2)
*/
int
nand_ecc_result(uint8_t status_register_byte);

/** Read and evaluate the status register ECC bits for data safety analysis.

  WARNING: Not all chips report the number of ECC bits in the status register.
  Further work is be necessary for chips which do not report 
  this status field, including the Macronix MX35LF1 family.

  @param user_data Flash SPI handle
  @param num_bits  On success, this is updated to contain the number of 
                   ECC-corrected bits reported by the chip.
  @return 0 if command and response completed
    succesfully, or error code
*/

int
nand_ecc_bits(nand_user_data_t *user_data, uint32_t* num_bits);

/** Get NAND ID.

    @param user_data Platform/user data handle
    @param[out] p_mfg_id Manufacturer ID
    @param[out] p_device_id Device ID

    @return 0 if successful, or other value if error
*/
int
nand_get_id(
  nand_user_data_t *user_data,
  uint8_t* p_mfg_id,
  uint16_t* p_device_id
  );

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
  );

/** Set feature register.

    @param user_data Flash SPI handle
    @param feature_reg Feature register to read
    @param data Data to write (1 byte)

    @return 0 if command and response completed
    succesfully, or error code
 */
int
nand_set_feature_reg(
  nand_user_data_t *user_data,
  feature_reg_t feature_reg,
  uint8_t data
  );

/** Unlock flash.

    After a power cycle, the flash chip is locked for writes (block
    protection bits BP0, BP1, BP2 in protection feature register are
    set). Before any writes can be performed, the flash must be
    unlocked.

    @param user_data Platform/user data handle

    @return 0 if successful, or other value if error
*/
int
nand_unlock(nand_user_data_t *user_data);

/** Enable/disable on-chip ECC.

    @param user_data Platform/user data handle
    @param enable True to enable ECC, false to disable

    @return 0 if successful, or other value if error
*/
int
nand_ecc_enable(nand_user_data_t *user_data, bool enable);

/** Generate the 24-bit "page address" from int block num, page num.

    @param block  block number, usually 2048 or less
    @param page   page number within that block, usually 64 or less
    @param block_addr_offset Number of left-shift bits of the block's address
           in the 24-bit page_addr.
    @return 24-bit address
*/
static inline uint32_t
nand_block_page_to_page_addr(uint32_t block, uint32_t page, uint8_t block_addr_offset)
{
  // Generate the page mask:
  uint32_t page_addr_mask = (1 << block_addr_offset) - 1;
  uint32_t page_addr = (block << block_addr_offset) | (page & page_addr_mask);

  return page_addr;
}

/** Generate the block and page from a 24-bit "page address".

    @param page_addr 24-bit page address
    @param block_addr_offset Number of left-shift bits of the block's address
           in the 24-bit page_addr.
    @param block  block number, usually 2048 or less
    @param page   page number within that block, usually 64 or less
*/
static inline void
nand_page_addr_to_block_page(uint32_t page_addr, uint8_t block_addr_offset, uint32_t* block, uint32_t* page)
{
  uint32_t page_addr_mask = (1 << block_addr_offset) - 1;
  *page  = page_addr & page_addr_mask;
  *block = page_addr >> block_addr_offset;
}

/** Read flash page.

    Note that (page_offset + data_len) must be less than
    (NAND_PAGE_SIZE + NAND_SPARE_SIZE)--you can't read past
    the end of a page into the next one.

    @param user_data Platform/user data handle
    @param block block number, 0..chip's block count
    @param page  page number, 0..chip's pages per block
    @param page_offset Offset within 4KB/2KB page
    @param[out] p_data Data buffer for reading
    @param data_len Size of data buffer (full 4K page + 256 spare = 4352 bytes)

    @return 0 if successful, or other value if error
*/
int
nand_read_page(
  nand_user_data_t *user_data,
  uint32_t block,
  uint32_t page,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  );

/** Write flash page.

    Note that (page_offset + data_len) must be less than
    (NAND_PAGE_SIZE + NAND_SPARE_SIZE)--you can't write past
    the end of a page into the next one.

    Also, with ECC on the last 128 bytes of the "spare area" are
    reserved for ECC data--writing to those bytes has no effect.

    @param user_data Platform/user data handle
    @param block block number, 0..chip's block count
    @param page  page number, 0..chip's pages per block
    @param page_offset Offset within page
    @param p_data Data buffer
    @param data_len Size of data buffer (full 4K page + 256 spare = 4352 bytes)

    @return 0 if successful, or other value if error
*/
int
nand_write_page(
  nand_user_data_t *user_data,
  uint32_t block,
  uint32_t page,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  );

/** Copy page: Copy a page into another address using internal cache.

    @param user_data Platform/user data handle
    @param src_page_addr Source 24-bit page address (incl. block shifted over some bits)
    @param dest_page_addr Destination 24-bit page address (incl. block shifted over some bits)

    NOTE: Unlike other functions, this does not take a block index, page index pair. 
    It takes the page address, as returned from nand_block_page_to_page_addr().

    @return 0 if successful, or other value if error
*/
int
nand_copy_page_from_cache(
  nand_user_data_t *user_data,
  uint32_t src_page_addr,
  uint32_t dest_page_addr
  );


/** Read flash page cache.

    Used internally, and for speed/integrity checks. Reads data
    from internal page cache, not from flash.

    Note that (page_offset + data_len) must be less than
    (NAND_PAGE_SIZE + NAND_SPARE_SIZE)--you can't read past
    the end of a page into the next one.

    @param user_data Platform/user data handle
    @param page_offset Offset within page
    @param[out] p_data Data buffer for reading
    @param data_len Size of data buffer (full 4K page + 256 spare = 4352 bytes)

    @return 0 if successful, or other value if error
*/
int
nand_read_page_from_cache(
  nand_user_data_t *user_data,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  );

/** Load flash page into the NAND chip's onboard cache, return ECC result. 
    Returns: <0, or One of NO_ERR (0), ECC_OK (1), or ECC_FAIL (-2)
*/
int
nand_read_page_into_cache(
  nand_user_data_t *user_data,
  uint32_t page_addr
  );

/** Write flash page cache.

    Used internally, and for speed/integrity checks. Writes data
    to internal page cache, but doesn't program the cache data to
    flash.

    Note that (page_offset + data_len) must be less than
    (NAND_PAGE_SIZE + NAND_SPARE_SIZE)--you can't write past
    the end of a page into the next one.

    Also, with ECC on the last 128 bytes of the "spare area" are
    reserved for ECC data--writing to those bytes has no effect.

    @param user_data Platform/user data handle
    @param page_offset Offset within 4KB page
    @param p_data Data buffer
    @param data_len Size of data buffer (full 4K page + 256 spare = 4352 bytes)

    @return 0 if successful, or other value if error
*/
int
nand_write_into_page_cache(
  nand_user_data_t *user_data,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  );


// Flush the NAND Chip internal buffer to the give 24-bit page_addr
int nand_program_page_cache(
  nand_user_data_t *user_data,
  uint32_t page_addr
  );

/** Check flash block.

    Blocks marked bad at factory are marked in first byte of spare
    area of first page.

    @param user_data Platform/user data handle
    @param row_addr 24-bit block address (0x000000-0x01FFFF)
    @param[out] p_good True if block is good, false if block is bad

    @return 0 if successful, or other value if error
*/
int
nand_block_status(
  nand_user_data_t *user_data,
  uint32_t page_addr,
  bool* p_good
  );

/** Erase block.

    @param user_data Platform/user data handle
    @param row_addr 24-bit block address (0x000000-0x01FFFF)

    @return 0 if successful, or other value if error
*/
int
nand_erase_block(
  nand_user_data_t *user_data,
  uint32_t page_addr
  );

/** Output detailed flash status.

    Prints decoded status registers to debug console. We keep this
    function in the flash driver so that we don't have to expose the
    internal register structures and constants outside the driver,
    since this is a debug-only function.

    @param user_data Handle from nand_init()  // TODO: How to pass this?

    @return 0 if successful
 */
int
nand_print_detailed_status(nand_user_data_t *user_data);

#ifdef __cplusplus
}
#endif

#endif  // NAND_H
