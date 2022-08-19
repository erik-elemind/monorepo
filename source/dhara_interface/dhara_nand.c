/*
 * dhara_nand.c
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Dhara nand interface.
 *
 */

#include "dhara_nand.h"
#include "nand.h"
#include "dhara_metadata_cache.h"
#include "dhara_utils.h"
#include <string.h>

// Set the log level for this file
#define LOG_LEVEL_MODULE  LOG_WARN
#include "loglevels.h"

/// Logging prefix
static const char* TAG = "dhara_nand";


static inline void dhara_page_addr_to_block_page(const struct dhara_nand *nand, dhara_page_t page_addr, uint32_t *block, uint32_t *page)
{
  uint32_t page_mask = (1 << nand->log2_ppb) - 1;

  *block =  page_addr >> nand->log2_ppb;
  *page = page_addr & page_mask;  
}

/* Is the given block bad? */
int dhara_nand_is_bad(const struct dhara_nand *nand, dhara_block_t block)
{
//  configASSERT( block>=0 && block<NAND_BLOCK_COUNT ); // TODO: restore check when file system issue is resolved.

  nand_user_data_t *user_data = (nand_user_data_t *)nand->user_data;
  int status = 0;

  uint32_t page_addr = nand_block_page_to_page_addr(block, 0, user_data->chipinfo->block_addr_offset);

  bool block_good = false;
  status = nand_block_status(user_data, page_addr, &block_good);

  if(status == 0){
    // 0xFF is the code indicating the block is good (originally set at the factory).
    if (block_good) {
      return 0;  // It's good
    } else {
      return 1;  // It's bad
    }
  }else{
    return -1;
  }
}

/* Mark bad the given block (or attempt to). No return value is
 * required, because there's nothing that can be done in response.
 */
void dhara_nand_mark_bad(const struct dhara_nand *nand, dhara_block_t block)
{
//  configASSERT( block>=0 && block<NAND_BLOCK_COUNT ); // TODO: restore check when file system issue is resolved.

  nand_user_data_t *user_data = (nand_user_data_t *)nand->user_data;
  int status;

  LOGI(TAG, "dhara_nand_mark_bad: block %lu", block);

  // The block holds 64 pages, of NAND_PAGE_SIZE (2048) + 127 bytes of
  // "spare" (metadata, incl. ECC bits).
  // The bad block marker is the first byte of the first page spare, meaning, the
  // the byte at offset NAND_PAGE_SIZE (2048).
  // A test for bad block is defined as that byte being "not 0xFF". So we write
  // a marker of 0x00 to get some protection against flipped bits (same as the factory).
  uint8_t is_bad_marker = 0x00;
  // the first page, page 0
  uint32_t page = 0;

  // Erase block before writing the is_bad_marker to page 0 at NAND_PAGE_SIZE.
  uint32_t page_addr = nand_block_page_to_page_addr(block, 0, user_data->chipinfo->block_addr_offset);
  status = nand_erase_block(user_data, page_addr);
  if (status != NAND_NO_ERR) {
    LOGE(TAG, "dhara_nand_mark_bad: %d.%d: "
      "nand_erase_block() error: %d", (int)block, (int)page, status);
    return;
  }

  status = nand_write_page(user_data, block, page,
    (1 << nand->log2_page_size), &is_bad_marker, sizeof(is_bad_marker));

  if (status != NAND_NO_ERR) {
    LOGE(TAG, "dhara_nand_mark_bad: %d.%d: "
      "nand_write_page() error: %d", (int)block, (int)page, status);
    return;
  }

  return;
}

/* Erase the given block. This function should return 0 on success or -1
 * on failure.
 *
 * The status reported by the chip should be checked. If an erase
 * operation fails, return -1 and set err to E_BAD_BLOCK.
 */
int dhara_nand_erase(const struct dhara_nand *nand, dhara_block_t block,
		     dhara_error_t *err)
{
//  configASSERT( block>=0 && block<NAND_BLOCK_COUNT );  // TODO: restore check when file system issue is resolved.

  nand_user_data_t *user_data = (nand_user_data_t *)nand->user_data;

  int status;

  // Erase block
  uint32_t page_addr = nand_block_page_to_page_addr(block, 0, user_data->chipinfo->block_addr_offset);
  uint32_t mask = (1 << user_data->chipinfo->block_addr_offset) - 1;

  dhara_metadata_cache_invalidate(page_addr, mask);
  if (!((user_data->layout_page_addr ^ page_addr) & ~mask))
      user_data->layout_page_addr = DHARA_PAGE_NONE;

  status = nand_erase_block(user_data, page_addr);
  if (status != NAND_NO_ERR) {
    LOGE(TAG, "dhara_nand_erase: block %d: "
      "nand_erase_block() error: %d", (int)block, status);
    dhara_set_error(err, DHARA_E_BAD_BLOCK);
    return -1;
  }

  *err = DHARA_E_NONE;
  return 0;
}


/* Program the given page. The data pointer is a pointer to an entire
 * page ((1 << log2_page_size) bytes). The operation status should be
 * checked. If the operation fails, return -1 and set err to
 * E_BAD_BLOCK.
 *
 * Pages will be programmed sequentially within a block, and will not be
 * reprogrammed.
 */
int dhara_nand_prog(const struct dhara_nand *nand, dhara_page_t page_addr,
		    const uint8_t *data,
		    dhara_error_t *err)
{
//  configASSERT( page_addr>=0 && page_addr<=NAND_PAGE_ADDR_MAX ); // TODO: restore check when file system issue is resolved.

  nand_user_data_t *user_data = (nand_user_data_t *)nand->user_data;

  dhara_metadata_cache_invalidate(page_addr, 0);

  // Write a whole page with no offset, by API
  const uint16_t page_offset = 0;  
  uint16_t data_size = (1 << nand->log2_page_size);  // Full page not including spare

  uint32_t block;
  uint32_t page;
  dhara_page_addr_to_block_page(nand, page_addr, &block, &page);

  int status;
  
  uint8_t *layout_buffer = user_data->layout_buffer;
  
  // Dhara requires that we have a page is_free() function to tell if a page has 
  // been written since last erase.
  // Our NAND chip has no way to tell us that from hardware.
  // One way to test is_free (and this is suggested by the Dhara docs) is to 
  // check the entire page buffer for 0xFF, but this has drawbacks:
  //  - this is only safe if writing all 0xFF to an erased page is a no-op
  //  - this requires us to read 2048 B just to get a boolean yes/no
  //
  // Another way to check (and this is suggested by Dhara's author in GitHub issues)
  // is to manually make use of the spare area to flag a page as being "not erased".
  // (This is very similar to UFFS's "seal byte" at the end of its spare data.)
  // We use this "seal byte" method. 
  // Our chip also requires that a page only be written once to use hardware ECC.
  // So to avoid copying Dhara's page data into a second buffer that also 
  // includes room for the spare data, we must init Dhara with a buffer big
  // enough to hold the spare byte.
  //
  // Finally, to avoid confusing the seal byte with a bad block marker on page 0,
  // we write 0xFF to the first spare byte, and then seal byte 0x00 as the 2nd spare byte.

  memcpy(layout_buffer, data, data_size);
  size_t layout_size = data_size;

  layout_buffer[layout_size] = 0xFF;  // First "not a bad block marker" spare byte.
  layout_size++;
  
  layout_buffer[layout_size] = 0x00;  // Second "seal byte which is not 0xFF" spare byte.
  layout_size++;

  LOGV(TAG, "dhara_nand_prog: block %d, page %d, data_size %d", (int)block, (int)page, (int)data_size);

  status = nand_write_page(user_data, block, page, page_offset, layout_buffer, layout_size);
  if (status != NAND_NO_ERR) {
    LOGE(TAG, "dhara_nand_prog: blk %d, pg %d data: "
      "nand_write_page() error: %d", (int)block, (int)page, status);
    dhara_set_error(err, DHARA_E_BAD_BLOCK);
    user_data->layout_page_addr = DHARA_PAGE_NONE;
    return -1;
  }
  user_data->layout_page_addr = (uint32_t)page_addr;

  dhara_set_error(err, DHARA_E_NONE);
  return 0;
}


/* Check that the given page is erased */
int dhara_nand_is_free(const struct dhara_nand *nand, dhara_page_t page_addr)
{
//  configASSERT( page_addr>=0 && page_addr<=NAND_PAGE_ADDR_MAX ); // TODO: restore check when file system issue is resolved.

  nand_user_data_t *user_data = (nand_user_data_t *)nand->user_data;

  uint32_t block;
  uint32_t page;
  dhara_page_addr_to_block_page(nand, page_addr, &block, &page);

  int status;
  
  // Read the 2nd byte of spare (the "seal byte") to see if it is 0xFF.
  // This is an invented convention we adopted in dhara_nand_prog().
  
  uint16_t page_offset = (1 << nand->log2_page_size);  // Go to last byte of non-spare data...
  // WARN: GigaDevice NAND chips must receive an even (not odd) offset:
  //page_offset++;   // ...and skip the first spare byte ("bad block marker").

  uint8_t data[2];  
  uint16_t data_size = 2;

  status = nand_read_page(user_data,
    block,
    page,
    page_offset, data, data_size);

  if (status != NAND_NO_ERR) {
    LOGE(TAG, "dhara_nand_is_free: dhara_nand_is_free() error: %d for block %lu, continuing anyway", status, block);
  }

  // 0xFF is the code indicating the block is erased.
  // We manually set the seal byte to 0x00 when programming (see dhara_nand_prog()).
  const uint8_t is_erased_mark = 0xFF;
  int is_erased = 1;
  
  if (data[1] == is_erased_mark) {
    is_erased = 1;  // It's erased (still at 0xFF since last block erase)
  } else {
    is_erased = 0;  // It's been written to
  }

  LOGV(TAG, "dhara_nand_is_free: is_erased(%lu.%lu): is_erased: %d", block, page, is_erased);

  return is_erased;
}

/* Read a portion of a page. ECC must be handled by the NAND
 * implementation. Returns 0 on sucess or -1 if an error occurs. If an
 * uncorrectable ECC error occurs, return -1 and set err to E_ECC.
 */
int dhara_nand_read(const struct dhara_nand *nand, dhara_page_t page_addr,
		    size_t offset, size_t length,
		    uint8_t *data,
		    dhara_error_t *err)
{
  // Disabling the configASSERT that checks the 'page_addr' argument, 
  // because on startup, if the file system was already corrupted,
  // dhara_nand_read() will get called with a page_addr that IS out of bounds,
  // and this will hang the device making all other (non-file-system) tests impossible.
//  configASSERT( page_addr>=0 && page_addr<=NAND_PAGE_ADDR_MAX );
//  configASSERT( offset>=0 && offset<NAND_PAGE_SIZE );

  nand_user_data_t *user_data = (nand_user_data_t *)nand->user_data;

  uint32_t block;
  uint32_t page;
  dhara_page_addr_to_block_page(nand, page_addr, &block, &page);
  
  int metadata_cache_was_hit;

  LOGV(TAG, "dhara_nand_read: block %d, page %d", (int)block, (int)page);

  int status = 0;
  dhara_set_error(err, DHARA_E_NONE);
  
  // Minimize page reads by using a metadata cache.
  if (length == DHARA_META_SIZE)
  {
    metadata_cache_was_hit = dhara_metadata_cache_get(page_addr, offset, data);
    if (metadata_cache_was_hit) {
      return 0;  // Success!
    }
  }

  uint8_t *layout_buffer = user_data->layout_buffer;
  size_t layout_size_bytes = user_data->layout_size_bytes;

  // Minimize page reads by using the layout buffer as a one-page cache:
  if (page_addr == (dhara_page_t)user_data->layout_page_addr)
  {
    memcpy(data, &(layout_buffer[offset]), length);
    return 0;  // Success!
  }
  
  // Uncached metadata reads tend to come from the same page. 
  // So if this is a metadata read, read the entire page into the layout buffer.
  if (length == DHARA_META_SIZE)
  {
    int page_offset = 0;
    status = nand_read_page(user_data, block, page, page_offset, layout_buffer, layout_size_bytes);
    // NAND_ECC_OK means some bits were corrected, maybe this block is going bad.
    if (status != NAND_NO_ERR && status != NAND_ECC_OK) {
      LOGE(TAG, "dhara_nand_read: %d.%d: data: "
        "nand_read_page() error: %d", (int)block, (int)page, status);
      return -1;
    }

    // Remember which page we have in RAM, for cached lookups:
    user_data->layout_page_addr = (uint32_t)page_addr;
    
    // Copy it into the destination buffer:
    memcpy(data, &(layout_buffer[offset]), length);
  } else {

    // Just do a normal NAND read directly into the supplies buffer
    status = nand_read_page(user_data, block, page, offset, data, length);
    // NAND_ECC_OK means some bits were corrected, maybe this block is going bad.
    if (status != NAND_NO_ERR && status != NAND_ECC_OK) {
      LOGE(TAG, "dhara_nand_read: %d.%d: data: "
        "nand_read_page() error: %d", (int)block, (int)page, status);
      return -1;
    }

  }

  // Put this into the metadata cache:
  if (length == DHARA_META_SIZE)
  {
    dhara_metadata_cache_set(page_addr, offset, data);
  }

  // If there was an ECC correction, return the DHARA_E_ECC_WARNING error
  // code to trigger a preemptive move. This is treated as a successful
  // read by the higher-level system.
  if (status == NAND_ECC_OK) {
    dhara_set_error(err, DHARA_E_ECC_WARNING);
    LOGD(TAG, "dhara_nand_read: %d.%d: ECC warning", (int)block, (int)page);

    // Recovery state doesn't actually get stored until we return, but
    // it's ok to set the signal now -- if it's consumed by the other
    // thread it'll need to acquire the lock before proceeding, and
    // that can't happen until after we have stored the recovery state,
    // since we're holding the lock.
    dhara_recovery_hint_set();
    return -1;
  }

  return 0;
}


/* Read a page from one location and reprogram it in another location.
 * This might be done using the chip's internal buffers, but it must use
 * ECC.
 */
int dhara_nand_copy(const struct dhara_nand *nand,
		    dhara_page_t src, dhara_page_t dst,
		    dhara_error_t *err)
{
//  configASSERT( src>=0 && src<=NAND_PAGE_ADDR_MAX ); // TODO: restore check when file system issue is resolved.
//  configASSERT( dst>=0 && dst<=NAND_PAGE_ADDR_MAX ); // TODO: restore check when file system issue is resolved.
  nand_user_data_t *user_data = (nand_user_data_t *)nand->user_data;
  int status = 0;
  int ret = 0;

  // This chip (GD5F4GQ6xExxG) has an internal data move operation.
  // However, Gigadevice have told us that it's only usable when both
  // the source and destination pages belong to the same plane (odd
  // blocks form one plane and even blocks another), and to the same die
  // (two contiguous 256 MB regions make up two dies).
  //
  // The test below checks that the page addresses have the same die
  // (0x20000) and same plane (0x40). If so, IDM is used. Otherwise, we
  // use the software fallback.
  if ((src ^ dst) & 0x20040) {
    uint8_t *layout_buffer = user_data->layout_buffer;
    size_t layout_size_bytes = user_data->layout_size_bytes;
    uint32_t block;
    uint32_t page;

    dhara_set_error(err, DHARA_E_NONE);
    user_data->layout_page_addr = DHARA_PAGE_NONE;

    // Read the source page into the layout buffer, noting ECC errors if
    // any
    dhara_page_addr_to_block_page(nand, src, &block, &page);
    status = nand_read_page(user_data, block, page, 0, layout_buffer, layout_size_bytes);
    if (status == NAND_ECC_OK) {
      ret = -1;
      dhara_set_error(err, DHARA_E_ECC_WARNING);
    } else if (status != NAND_NO_ERR) {
      dhara_set_error(err, DHARA_E_ECC);
      LOGE(TAG, "dhara_nand_copy: %d.%d: data: "
	"nand_read_page() error: %d", (int)block, (int)page, status);
      return -1;
    }

    // Write the buffered page back into the new position. Remember to
    // include the first two spare bytes (bad block and free-page
    // markers, which should be 0xff and 0x00 respectively).
    dhara_page_addr_to_block_page(nand, dst, &block, &page);
    status = nand_write_page(user_data, block, page, 0, layout_buffer,
        (1 << (nand->log2_page_size + 2)));
    if (status != NAND_NO_ERR) {
      LOGE(TAG, "dhara_nand_copy: blk %d, pg %d data: "
	"nand_write_page() error: %d", (int)block, (int)page, status);
      dhara_set_error(err, DHARA_E_BAD_BLOCK);
      return -1;
    }

    // Both pages contain the same information, but the destination is
    // more likely to be a useful cache tag. The source is probably at the
    // tail of the journal and likely to be erased soon.
    user_data->layout_page_addr = (uint32_t)dst;
  } else {
    status = nand_copy_page_from_cache(user_data, (uint32_t)src, (uint32_t)dst);
    if (status != NAND_NO_ERR && status != NAND_ECC_OK) {
      LOGE(TAG, "dhara_nand_copy: status %d, returning -1 (err: DHARA_E_BAD_BLOCK)", status);
      dhara_set_error(err, DHARA_E_BAD_BLOCK);
      return -1;
    }

    if (status == NAND_ECC_OK) {
      ret = -1;
      dhara_set_error(err, DHARA_E_ECC_WARNING);
    }
  }

  return ret;
}

