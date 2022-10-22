/*
 * flash_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for NAND SPI flash.
 *
 */
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "fsl_gpio.h"
#include "loglevels.h"

#include "nand_W25N04KW.h"
#include "board_config.h"
#include "command_helpers.h"

#include "flash_commands.h"

#include "noise_test.h"

// Create user data struct
static nand_user_data_t g_nand_handle = {
  .chipinfo = &nand_chipinfo,
};

void
flash_hello(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  // Run a Get Features command to read the Protection register.
  // 0xA0: Protection register (CMP, INV, BP0/1/2, BRWD)
  uint8_t data;
  int status = nand_get_feature_reg(&g_nand_handle, 0xA0, &data);

  printf("Status: %d, Reply: 0x%x\n", status, data);
}

void
flash_id(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  uint8_t mfg_id;
  uint16_t device_id;
  int status = nand_get_id(&g_nand_handle, &mfg_id, &device_id);
  if (status == 0) {
    printf("Flash ID: mfg_id: 0x%x, device_id: 0x%x\n", mfg_id, device_id);
  }
  else {
    printf("Flash ID error: %d\n", status);
  }
}

void
flash_status(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  printf("Flash status:\n");
  nand_print_detailed_status(&g_nand_handle);
}

#if 1

void
flash_unlock(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  int status = nand_unlock(&g_nand_handle);
  if (status == 0) {
    printf("Flash unlocked.\n");
  }
  else {
    printf("Flash unlock error: %d\n", status);
  }
}

void
flash_read_page(int argc, char **argv)
{
  if (argc != 2) {
    printf("Error: Missing page address\n");
    printf("Usage: %s <page_addr>\n", argv[0]);
    return;
  }

  // Get address
  uint32_t address;
  if (!parse_uint32_arg_max(argv[0], argv[1],
      NAND_PAGE_ADDR_MAX, &address)) {
    return;
  }

  // Get block and page numbers
  uint32_t block, page;
  nand_page_addr_to_block_page(
    address, g_nand_handle.chipinfo->block_addr_offset, &block, &page);

  uint8_t* data = malloc(NAND_PAGE_SIZE + NAND_SPARE_SIZE);
  if (data == NULL) {
    printf("Unable to allocate data buffer!\n");
    return;
  }

  int status = nand_read_page(&g_nand_handle, block, page, 0, data,
      NAND_PAGE_SIZE + NAND_SPARE_SIZE);
  if (0 == status) {
    printf("Flash page data:\n");
    hex_dump(data, NAND_PAGE_SIZE, 0);

    printf("Flash page spare:\n");
    hex_dump(data, NAND_PAGE_SIZE + NAND_SPARE_SIZE, NAND_PAGE_SIZE);

    if ((address % 64) == 0) {
      // For first page in block, first byte of spare area has "bad page" mark
      if (data[NAND_PAGE_SIZE] != 0xFF) {
        printf("Flash block is bad: %02x\n", data[NAND_PAGE_SIZE]);
      }
      else {
        printf("Flash block is good\n");
      }
    }

    // Compare the data read to the data written by the flash_page_write function
    printf("Comparing flash data read vs data written by function \"flash_write_page\".\n");
    uint16_t expected_data16 = 0;
    uint8_t *p_expected_data8 = (uint8_t*) &expected_data16;
    bool invert = false;
    for (int i = 0; i < NAND_PAGE_SIZE / sizeof(uint16_t); i++) {
      expected_data16 = i * 2;
      if (invert) {
        expected_data16 = ~expected_data16;
      }
      if (p_expected_data8[0] != data[i * 2] || p_expected_data8[1] != data[i * 2 + 1]) {
        printf("Failed, data mismatch found!\n");
        goto flash_read_page_exit;
      }
    }
    printf("Success, data comparison passed.\n");
  } // end if (status == 0)
  else {
    printf("Flash read page error: %d\n", status);
  }

  flash_read_page_exit:
  free(data);
}

void
flash_write_page(int argc, char **argv)
{
  if ((argc < 2) || (argc > 3)) {
    printf("Error: Incorrect number of arguments\n");
    printf("Usage: %s <page_addr> [invert]\n", argv[0]);
    return;
  }

  // Get address
  uint32_t address;
  if (!parse_uint32_arg_max(argv[0], argv[1],
      NAND_PAGE_ADDR_MAX, &address)) {
    return;
  }

  // Get block and page numbers
  uint32_t block, page;
  nand_page_addr_to_block_page(
    address, g_nand_handle.chipinfo->block_addr_offset, &block, &page);

  // Get "invert" flag
  bool invert = false;
  if (argc >= 3) {
    invert = !strncmp("inv", argv[2], sizeof("inv"));
  }

  // Populate test data (write 16-bit address)
  uint8_t* data= malloc(NAND_PAGE_SIZE);
  if (data != NULL) {
    uint16_t* p_data16 = (uint16_t*)data;
    for (int i = 0; i < NAND_PAGE_SIZE/sizeof(uint16_t); i++) {
      p_data16[i] = i*2;
      if (invert) {
        p_data16[i] = ~p_data16[i];
      }
    }
    hex_dump(data, NAND_PAGE_SIZE, 0);

    int status = nand_write_page(&g_nand_handle, block, page, 0,
      data, NAND_PAGE_SIZE);
    if (status != 0) {
      printf("Flash write page error: %d\n", status);
    }

    free(data);
  }
  else {
    printf("Unable to allocate data buffer!\n");
  }
}

void
flash_check_block(int argc, char **argv)
{
  if (argc != 2) {
    printf("Error: %s takes 1 argument\n", argv[0]);
    return;
  }

  uint32_t block_idx = 0;
  if (!parse_uint32_arg_max(argv[0], argv[1],
      NAND_BLOCK_COUNT, &block_idx)) {
    printf("Error: Could not parse <block addr> %s", argv[1]);
    return;
  }

#if 0
  // Reduce logging
  int log_level_orig = g_runtime_log_level;
  g_runtime_log_level = 1;
#endif

    bool block_good = false;
    uint32_t page_addr = block_idx << NAND_BLOCK_ADDR_OFFSET;

    // Shift the block addr into position in the 24-bit address.
    // And check the block.
    int status = nand_block_status(&g_nand_handle,
      page_addr, &block_good);
    if (status == 0) {
      if (block_good) {
        //printf(".");
        printf("\nBlock good, page 0x%06lx (%lu), block %lu\n", page_addr, page_addr, block_idx);
      }
      else {
        printf("\nBlock bad, page 0x%06lx (%lu), block %lu\n", page_addr, page_addr, block_idx);
      }
    }
    else {
      printf("\nError reading status for page 0x%06lx (%lu), block %lu\n", page_addr, page_addr, block_idx);
    }

#if 0
  g_runtime_log_level = log_level_orig;
#endif
}

void
flash_check_blocks(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  uint32_t blocks_good = 0;
  uint32_t blocks_bad = 0;
  uint32_t blocks_failed = 0;

#if 0
  // Reduce logging
  int log_level_orig = g_runtime_log_level;
  g_runtime_log_level = 1;
#endif

  for (uint32_t block_idx = 0; block_idx <= NAND_BLOCK_COUNT; block_idx++) {
    bool block_good = false;
    uint32_t page_addr = block_idx << NAND_BLOCK_ADDR_OFFSET;

    // Shift the block addr into position in the 24-bit address.
    // And check the block.
    int status = nand_block_status(&g_nand_handle,
      page_addr, &block_good);
    if (status == 0) {
      if (block_good) {
        //printf(".");
        blocks_good++;
        printf("\nBlock good, page 0x%06lx (%lu), block %lu\n", page_addr, page_addr, block_idx);
      }
      else {
        blocks_bad++;
        printf("\nBlock bad, page 0x%06lx (%lu), block %lu\n", page_addr, page_addr, block_idx);
      }
    }
    else {
      blocks_failed++;
      printf("\nError reading status for page 0x%06lx (%lu), block %lu\n", page_addr, page_addr, block_idx);
    }
  }

#if 0
  g_runtime_log_level = log_level_orig;
#endif

  printf("%lu of %lu bad. %lu failed.\n",
    blocks_bad, blocks_good + blocks_bad, blocks_failed);
}

/* This function performs reads of each page and checks for ECC errors.
 * Experimentally, it can also update the bad block markers based on this result.
 */
void
flash_check_blocks_ecc(int argc, char **argv) {
  // Arbitrary max ECC errors per block
  static const uint32_t block_ecc_bits_max = 10;

  // This flag indicates we will erase the page and re-apply the Bad Block Marker.
  // Only do this if you think your flash chip has been erased incorrectly 
  // or the Bad Block Marks have been overwritten.
  bool erase_recover = false;

//  if (argc != 1) {
//    printf("Error: %s takes no arguments\n", argv[0]);
//    return;
//  }

  if (argc == 2) {
    if (argv[1][0] == '1') {
      erase_recover = true;
    }
  }

  uint32_t blocks_written_good = 0;
  uint32_t blocks_written_bad = 0;
  int status;

#if 0
  // Reduce logging
  int log_level_orig = g_runtime_log_level;
  g_runtime_log_level = 1;
#endif

  for (uint32_t block = 0; block <= NAND_BLOCK_COUNT; block++) {
    bool block_good = false;
    uint32_t page_addr = block << NAND_BLOCK_ADDR_OFFSET;

    // Shift the block addr into position in the 24-bit address.
    // And check the block.
    status = nand_block_status(&g_nand_handle,
      page_addr, &block_good);
    if (status != 0) {
      printf("flash_fix_blocks: nand_block_status(): on page 0x%06lx (%lu): error: %d ", page_addr, page_addr, status);
      goto flash_check_blocks_ecc_exit;
    }

    uint32_t block_ecc_bits_corrected = 0;

    for (uint32_t page_idx = 0;
        page_idx < NAND_PAGES_PER_BLOCK;
        page_idx++, page_addr++) {
      // Read a page(s) from the block into the internal cache.
      // This loads the ECC value.
      status = nand_read_page_into_cache(&g_nand_handle, page_addr);
      if (status == NAND_NO_ERR ||
          status == NAND_ECC_OK ||
          status == NAND_ECC_FAIL) {
        // Get the actual number of ECC corrected bits
        uint32_t ecc_bits_corrected;
        status = nand_ecc_bits(&g_nand_handle, &ecc_bits_corrected);
        if (status) {
          printf("flash_fix_blocks: nand_ecc_bits(): on page 0x%06lx (%lu): error: %d ", page_addr, page_addr, status);
          goto flash_check_blocks_ecc_exit;
        }
        else {
          if (ecc_bits_corrected > 0) {
            //printf("flash_fix_blocks: page 0x%06lx (%lu) has %lu errors\n", page_addr, page_addr, ecc_bits_corrected);
            block_ecc_bits_corrected += ecc_bits_corrected;
          }
        }
      }
      else {
        printf("flash_fix_blocks: nand_read_page_into_cache(): on page 0x%06lx (%lu): error: %d ", page_addr, page_addr, status);
        goto flash_check_blocks_ecc_exit;
      }
    }

    if (block_ecc_bits_corrected) {
      printf("flash_fix_blocks: block 0x%06lx (%lu) has %lu corrected bits. block_good: %d\n",
          block, block, block_ecc_bits_corrected, block_good);

      if (erase_recover) {
        // If block ECC count is high, but marked as good, then
        // mark it bad.
        if (block_ecc_bits_corrected > block_ecc_bits_max && block_good) {
          uint8_t bbm_byte = 0;
          status = nand_write_page(&g_nand_handle, block, 
            0, NAND_PAGE_SIZE, // first spare byte of the first page
            &bbm_byte, sizeof(bbm_byte));
          if (status != 0) {
            printf("Flash write block error: %d\n", status);
          }
          blocks_written_bad++;
        }

        // If block ECC count is low, but marked as bad, just erase
        // the block to restore it.
        else if (block_ecc_bits_corrected <= block_ecc_bits_max && !block_good) {
          uint32_t page_addr = nand_block_page_to_page_addr(block, 0, g_nand_handle.chipinfo->block_addr_offset);
          status = nand_erase_block(&g_nand_handle, page_addr);
          if (status != 0) {
            printf("Flash erase block error: %d\n", status);
          }
          blocks_written_good++;
        }
      }
    }
  }

flash_check_blocks_ecc_exit:
#if 0
  g_runtime_log_level = log_level_orig;
#endif

  printf("Blocks marked good: %lu. Blocks marked bad: %lu.\n",
    blocks_written_good, blocks_written_bad);
}


void
flash_erase_block(int argc, char **argv)
{
  if (argc != 2) {
    printf("Error: Missing block address\n");
    printf("Usage: %s <block_addr>\n", argv[0]);
    return;
  }

  // Get address
  uint32_t block_addr;
  if (!parse_uint32_arg_max(argv[0], argv[1],
      NAND_BLOCK_COUNT, &block_addr)) {
    printf("Error: Could not parse <block addr> %s", argv[1]);
    return;
  }

  uint32_t page = 0;
  uint32_t page_addr = nand_block_page_to_page_addr(block_addr, page, g_nand_handle.chipinfo->block_addr_offset);

  // erase block
  int status = nand_erase_block(&g_nand_handle, page_addr);
  if (status != 0) {
    printf("Error: Flash erase block error: %d\n", status);
  }else{
    printf("Flash erase block success\n");
  }
}



#if 0
page_addr == 2047*NAND_PAGES_PER_BLOCK ||
    page_addr == 1537*NAND_PAGES_PER_BLOCK ||
    page_addr == 1536*NAND_PAGES_PER_BLOCK ||
    page_addr == 1365*NAND_PAGES_PER_BLOCK ||
    page_addr == 1022*NAND_PAGES_PER_BLOCK ||
    page_addr == 1020*NAND_PAGES_PER_BLOCK ||
    page_addr == 734*NAND_PAGES_PER_BLOCK ||
    page_addr == 732*NAND_PAGES_PER_BLOCK ||
    page_addr == 730*NAND_PAGES_PER_BLOCK ||
    page_addr == 384*NAND_PAGES_PER_BLOCK ||
    page_addr == 181*NAND_PAGES_PER_BLOCK
#endif

void
flash_erase_blocks(int argc, char **argv) {
  printf("Starting \"%s\".\n",argv[0]);
  int status;
  uint16_t num_blocks = 0;
  uint32_t start_block = 0;
  uint32_t end_block;
  if (num_blocks == 0) {
    end_block = NAND_BLOCK_COUNT;
  }
  else {
    end_block = num_blocks;
  }

  for (uint32_t block = start_block; block <= end_block; block++) {
    uint32_t page = 0;
    uint32_t page_addr = nand_block_page_to_page_addr(block, page, g_nand_handle.chipinfo->block_addr_offset);

    // erase block
    status = nand_erase_block(&g_nand_handle, page_addr);
    if (status != 0) {
      printf("Error: Flash erase block error: %d\n", status);
    }

#if 1
    bool read_good;
    // check for the good block mark
    status = nand_block_status(&g_nand_handle, page_addr, &read_good);
    if (status != 0) {
//      printf("flash_mark_blocks_good: nand_block_status(): on page_addr 0x%06lx (%lu), page %lu of block: %lu : error: %d\n",
//          page_addr, page_addr, page, block, status);
    }

    if(!read_good){
      printf("x");
      fflush(stdout);
//      printf("flash_mark_blocks_good: page 0x%06lx (%lu), page %lu of block %lu : bad.\n",
//                page_addr, page_addr, page, block);
    }else{
      printf(".");
      fflush(stdout);
//      printf("flash_mark_blocks_good: page 0x%06lx (%lu), page %lu of block %lu : good.\n",
//                page_addr, page_addr, page, block);
    }

    if (block % 64 == 0)printf("\n");
#endif
  }

//  flash_mark_blocks_good_command_exit:
  printf("Completed \"%s\".\n",argv[0]);
}

void
flash_mark_bad_block(int argc, char **argv) {
  if (argc != 2) {
    printf("Error: %s takes 1 argument\n", argv[0]);
    return;
  }

  uint32_t block_idx = 0;
  if (!parse_uint32_arg_max(argv[0], argv[1],
      NAND_BLOCK_COUNT, &block_idx)) {
    printf("Error: Could not parse <block addr> %s", argv[1]);
    return;
  }

  uint8_t bad_mark = 0x00;
  nand_write_page(&g_nand_handle, block_idx, 0, NAND_PAGE_SIZE, &bad_mark, sizeof(bad_mark));

    printf("Completed \"%s\".\n",argv[0]);
}


static uint16_t
data_for_page(uint32_t page, bool invert)
{
  uint16_t data;

  if (page <= UINT16_MAX) {
    data = (uint16_t)page;
  }
  else {
    /* If page address > 16 bits, invert it (so that we don't
       write same data to pages with address only 1 bit off). */
    data = (uint16_t)~page;
  }

  if (invert) {
    data = ~data;
  }

  return data;
}

#if 0
static void
spin()
{
  static char spinner[] = "|/-\\";
  static uint8_t index = 0;

  putchar('\b');
  putchar(spinner[index++ % (sizeof(spinner)-1)]);
  fflush(stdout);
}
#endif

enum {
  FLASH_TEST_BLOCKS_PER_LINE = 64,
  FLASH_TEST_PAGES_PER_LINE =
    NAND_PAGES_PER_BLOCK * FLASH_TEST_BLOCKS_PER_LINE
};

static void
flash_test_internal(uint32_t max_iterations, uint16_t num_blocks, bool mark_bad_blocks)
{
  #if 0
  printf("Test disabled.\n");
  #else
  status_t status;

  uint32_t iterations = 0;
  bool invert = false;
  uint32_t good_pages = 0;
  uint32_t old_bad_blocks = 0;
  uint32_t new_bad_blocks = 0;

  uint32_t start_page_addr = 0;
  uint32_t end_page_addr;
  if (num_blocks == 0) {
    end_page_addr = NAND_PAGE_ADDR_MAX;
  }
  else {
    end_page_addr = num_blocks * NAND_PAGES_PER_BLOCK - 1;
  }

  uint8_t* buffer = malloc(NAND_PAGE_SIZE);
  if (buffer == NULL) {
    printf("Error: Unable to allocate buffer!\n");
    return;
  }

  do {
    for (uint32_t page_addr = start_page_addr;
         page_addr <= end_page_addr;
         page_addr ++) {

      uint32_t block;
      uint32_t page;
      nand_page_addr_to_block_page(page_addr, g_nand_handle.chipinfo->block_addr_offset, &block, &page);

      // Check if we are on the first page of a block.
      if (page_addr % (NAND_PAGES_PER_BLOCK) == 0) {
        printf("flash_test on block %lu (page_addr 0x%06lx - 0x%06lx)\n", block, page_addr, page_addr+NAND_PAGES_PER_BLOCK-1);

        // check block status
        bool good = false;
        status = nand_block_status(&g_nand_handle, page_addr, &good);
        if (status != kStatus_Success) {
          printf("  nand_block_status(), error: %ld\n", status);
          goto flash_test_internal_exit;
        }
        if (!good) {
          // Skip bad block (next block - 1, since page will get incremented)
          printf("\nflash_test: skipping bad block, page_addr: 0x%06lx, block: %lu\n", page_addr, page_addr>>NAND_PAGES_PER_BLOCK_LOG2);
          page_addr += NAND_PAGES_PER_BLOCK - 1;
          old_bad_blocks++;
          continue;
        }

        // erase block
        status = nand_erase_block(&g_nand_handle, page_addr);
        if (status != kStatus_Success) {
          printf("  nand_erase_block() error: %ld\n", status);
          goto flash_test_internal_exit;
        }
      } // end if statement per block



      // Write
      // Calculate data word to write
      uint16_t data = data_for_page(page_addr, invert);

      // Populate buffer
      for (int i = 0; i < NAND_PAGE_SIZE; i += sizeof(data)) {
        *(uint16_t*)(buffer + i) = data;
      }

      // Write data
      status = nand_write_page(&g_nand_handle, block, page, 0, buffer, NAND_PAGE_SIZE);

      if (status != 0) {
        printf("  nand_write_page() error: %ld\n", status);
        goto flash_test_internal_exit;
      }


      // Read
      memset(buffer,0,NAND_PAGE_SIZE);
      status = nand_read_page(&g_nand_handle, block, page, 0, buffer, NAND_PAGE_SIZE);

      switch(status){
      case NAND_NO_ERR:
        // do nothing
        break;
      case NAND_ECC_OK:
        printf("  nand_read_page() error: %ld (ecc recovered)\n", status);
//        goto flash_test_internal_exit;
        break;
      case NAND_ECC_FAIL:
        printf("  nand_read_page() error: %ld (ecc unrecoverable)\n", status);
        if(mark_bad_blocks){
          uint8_t bad_mark = 0x00;
          nand_write_page(&g_nand_handle, block, 0, NAND_PAGE_SIZE, &bad_mark, sizeof(bad_mark));
          printf("  marking bad block, page_addr: 0x%06lx, block: %lu\n", page_addr, block);
          // Move to the next block, but decrement the page number because the next for loop
          // will increment it.
          page_addr = nand_block_page_to_page_addr(block+1, 0, g_nand_handle.chipinfo->block_addr_offset) - 1;
          new_bad_blocks++;
          continue;
        }else{
          goto flash_test_internal_exit;
        }
        break;
      default:
        printf(" nand_read_page() error: %ld (unknown)\n", status);
        goto flash_test_internal_exit;
      }

      // Check for expected data
      for (int i = 0; i < NAND_PAGE_SIZE; i += sizeof(data)) {
        if (*(uint16_t*)(buffer + i) != data) {
          printf("  flash_test: verify error: page 0x%06lx, addr 0x%04x:\n",
              page_addr, i);
          printf("  expected 0x%04x, got 0x%04x\n",
            data, *(uint16_t*)(buffer + i));
          hex_dump(buffer, NAND_PAGE_SIZE, 0);
          if(mark_bad_blocks){
            uint8_t bad_mark = 0x00;
            nand_write_page(&g_nand_handle, block, 0, NAND_PAGE_SIZE, &bad_mark, sizeof(bad_mark));
            printf("  flash_test: marking bad block, page_addr: 0x%06lx, block: %lu\n", page_addr, block);
            // Move to the next block, but decrement the page number because the next for loop
            // will increment it.
            page_addr = nand_block_page_to_page_addr(block+1, 0, g_nand_handle.chipinfo->block_addr_offset) - 1;
            new_bad_blocks++;
            continue;
          }else{
            goto flash_test_internal_exit;
          }
        }
      }

      good_pages++;
    } // end for loop


    printf("  flash_test result: good_blocks: %lu, old_bad_blocks: %lu, new_bad_blocks: %lu", good_pages>>NAND_PAGES_PER_BLOCK_LOG2, old_bad_blocks, new_bad_blocks);

    // Switch sense the next time around to exercise all the bits
    invert = !invert;

    // Count the number of times we've iterated through the entire flash array
    iterations++;

  } while (max_iterations>0 && iterations<max_iterations);

  // Use label to ensure we always free buffer
 flash_test_internal_exit:
  free(buffer);
  #endif
}

void
flash_test_complete(int argc, char **argv)
{
  if (argc != 1 && argc!=3) {
    // ToDo: Fix this error message now that this command is parameterized
    printf("Error: %s takes either no arguments or 2 arguments\n", argv[0]);
    return;
  }

  uint32_t max_iterations = 1;
  uint8_t mark_bad_block = 0;
  if(argc==3){
    if (!parse_uint32_arg_max(argv[0], argv[1],
        1, &max_iterations)) {
      return;
    }
    if (!parse_uint8_arg_max(argv[0], argv[2],
        1, &mark_bad_block)) {
      return;
    }
  }

  // ToDo: Fix this message now that this command is parameterized
  printf("Continuous complete flash test: \n");
  printf("  full erase/write/read cycles over all blocks\n");
  printf("Warning: each write or read cycle will take up to 2.5 hours!\n\n");

  flash_test_internal(max_iterations, 0, mark_bad_block!=0);
}

void
flash_test_speed(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  /* Note that we need num_blocks to be a multiple of
     FLASH_TEST_BLOCKS_PER_LINE, since flash_test_internal() only
     prints speed at the end of each line. */
  uint16_t num_blocks = FLASH_TEST_BLOCKS_PER_LINE;

  printf("Flash speed test:");
  printf("  one erase/write/read cycle over %d blocks\n", num_blocks);

  flash_test_internal(1, num_blocks, false);
}

void
flash_test_spi(int argc, char **argv)
{
  #if 0
  printf("Test disabled.\n");
  #else
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  printf("Continuous flash SPI test: \n");
  printf("  write/read to internal page cache (don't program flash)\n");

  int status;

  uint32_t count = 0;
  bool invert = false;

  /* Allocate buffer here, but don't worry about freeing--test runs
     continuously, unless an error occurs. */
  uint8_t* data = malloc(NAND_PAGE_SIZE);
  if (data == NULL) {
    printf("Error: unable to allocate buffer!\n");
    return;
  }

  while (true) {
    // Print "." every 64 iterations, newline every 64*64 iterations
    if (count % 64 == 0) {
      if (count % (64*64) == 0) {
        printf("\b \n%5ld:  ", count); // extra space for spinner placeholder
      }

      // Print "." progress indicator (erase spinner, space after for spinner)
      printf("\b. ");
    }

    // Set data to incrementing bytes (inverted or non-inverted)
    uint16_t* p_data16 = (uint16_t*)data;
    for (int i = 0; i < NAND_PAGE_SIZE/sizeof(uint16_t); i++) {
      p_data16[i] = i*2;
      if (invert) {
        p_data16[i] = ~p_data16[i];
      }
    }

    // Write data to page cache
    status = nand_write_into_page_cache(&g_nand_handle, 0,
      data, NAND_PAGE_SIZE);
    if (status != kStatus_Success) {
      printf("Flash write page cache error: %d\n", status);
      free(data);
      return;
    }

    // Zero data buffer
    memset(data, 0, NAND_PAGE_SIZE);

    // Read data from page cache
    status = nand_read_page_from_cache(&g_nand_handle, 0,
      data, NAND_PAGE_SIZE);
    if (status != kStatus_Success) {
      printf("Flash read page cache error: %d\n", status);
      free(data);
      return;
    }

    // Verify data
    p_data16 = (uint16_t*)data;
    for (int i = 0; i < NAND_PAGE_SIZE/sizeof(uint16_t); i++) {
      uint16_t address = i*2;
      uint16_t expected = invert ? ~address : address;
      if (p_data16[i] != expected) {
        printf("Error: Address %06x: expected %06x, read %06x\n",
          address, expected, p_data16[i]);
        free(data);
        return;
      }
    }

    // Spin progress spinner
    //spin();

    // Switch sense the next time around to exercise all the bits
    invert = !invert;

    count++;
  }
  free(data);
  #endif
}



#if (defined(ENABLE_NOISE_TEST) && (ENABLE_NOISE_TEST > 0U))

void
noise_test_flash_start_command(int argc, char **argv)
{
  if (argc == 1) {
    noise_test_flash_start( (char*)"rw" );
  } else if (argc == 2) {
    noise_test_flash_start( argv[1] );
  } else {
    printf("Error: %s expects 0 or 1 argument: A string defining the testing pattern, using characters r/w/R/W\n", argv[0]);
    return;
  }
}

void
noise_test_flash_stop_command(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }
  noise_test_flash_stop();
}

#endif
#endif
