/*
 * nand_commands.c
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Shell commands for UFFS.
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"

#include "nand_W25N04KW.h"

#include "board_config.h"

#include "utils.h"
#include "loglevels.h"

static const char* TAG = "nand_cmd";

#define MAX_PATH_LENGTH 128

#define UFFS_TEST_FILEPATH "/testfile.txt"
#define NAND_TESTS_USE_MALLOC 1

// Create user data struct
static nand_user_data_t g_nand_handle = {
  .chipinfo = &nand_chipinfo,
};

void
nand_id_command(int argc, char **argv)
{
  uint8_t mfg_id;
  uint16_t device_id;

  nand_get_id(&g_nand_handle, &mfg_id, &device_id);
  printf("nand_get_id: mfg_id 0x%x, device_id 0x%x\n", mfg_id, device_id);
}

void
nand_info_command(int argc, char **argv)
{
  nand_print_detailed_status(&g_nand_handle);
}

#if 0  // TODO: Make a utils.h for this
static void print_buf(uint8_t *buf, int count) {
  int line_wrap_count = 30;
  int index;
  for (index = 0 ; index < count ; index++) {
    printf("%x ", buf[index]);

    if (index % line_wrap_count == 0) {
      printf("\n");
    }
  }
  printf("\n");
};
#endif


void
nand_write_read_command(int argc, char **argv)
{
  int status;
  int block;
  int page = 0;
  // data_size does not include the ECC region used by the hardware, so /2:
//FIXME derek test:  uint32_t data_size = (NAND_PAGE_SIZE + (NAND_SPARE_SIZE/2));
  uint32_t data_size = (NAND_PAGE_SIZE + 2);

#ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
  uint8_t *data = malloc(NAND_PAGE_SIZE + NAND_SPARE_SIZE);
  uint8_t *data_out = malloc(NAND_PAGE_SIZE + NAND_SPARE_SIZE);

  if ((data == NULL) || (data_out == NULL)) {
    LOGE("nand_write_read", "malloc failure, returned NULL");
  }
#else
  // If these are not static, you'll need a huge Task stack size to push them.
  static uint8_t data[(NAND_PAGE_SIZE + NAND_SPARE_SIZE)];
  static uint8_t data_out[(NAND_PAGE_SIZE + NAND_SPARE_SIZE)];
#endif

  uint16_t page_offset; // write the entire page with spare
  uint32_t index;

  int run_count = 0;
  for (run_count = 0 ; run_count < 1; run_count++) {

    // Populate some non-zero values:
    for (index = 0 ; index < data_size; index++) {
        data[index] = index & 0xFF;
    }

    // ...and make the spare data count down instead of up, for easy identification
    for (index = 0 ; index < NAND_SPARE_SIZE; index++) {
        data[NAND_PAGE_SIZE + index] = (NAND_SPARE_SIZE - index) & 0xFF;
    }

    // ...but don't overwrite the is_good_mark with a non-0xFF mark:
    // Emulate a bad block marker and seal byte:
    data[NAND_PAGE_SIZE] = 0xFF;
    data[NAND_PAGE_SIZE + 1] = 0x00;

    //data_size = NAND_PAGE_SIZE + NAND_SPARE_SIZE;
    data_size = NAND_PAGE_SIZE;

    // A marker value:
    for (index = 0 ; index < data_size; index++) {
        data_out[index] = 0xa5;
    }


    block = 0;
    page = 0;

    page_offset = 0;
    int total_bad_blocks_count = 0;
    int marked_bad_blocks_count = 0;

    TickType_t t0 = xTaskGetTickCount();

//    for (block = 0 ; block < NAND_BLOCK_COUNT; block++) {
    for (block = 0 ; block < 30; block++) {

      // First see if this is a bad block before erasing it for a write test.
      // We don't want to lose previously identified bad blocks.
      // Also, minimize prints for speed:
      //printf("Read from %d:%d:%d (%d B): \n", (int)block, (int)page, (int)page_offset, (int)data_size);
      if (block % 64 == 0)printf("\n");

      // The datasheet says to turn off ECC before checking this byte. There must
      // be some slim possibility that it will ECC "correct" it from 0x00 to 0xFF.
      status = nand_ecc_enable(&g_nand_handle, FALSE);
      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_ecc_enable(1): %d for block %u, continuing anyway", status, block);
      }

      const uint8_t is_good_mark = 0xFF;
      int is_bad_block = 1;

      page_offset = NAND_PAGE_SIZE;

      status = nand_read_page(&g_nand_handle,
        block,
        page,
        page_offset, data_out, 1);

      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_read_page() error: %d for block %u, continuing anyway", status, block);
      }

      status = nand_ecc_enable(&g_nand_handle, TRUE);   // Turn it back on
      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_ecc_enable(2) error: %d for block %u, continuing anyway", status, block);
      }

      // 0xFF is the code indicating the block is good (originally set at the factory).
      if (data_out[0] == is_good_mark) {
        is_bad_block = 0;  // It's good
      } else {
        is_bad_block = 1;  // It's bad
      }

      if (is_bad_block) {
        total_bad_blocks_count++;
        //LOGE(TAG, "IsBadBlock: Skipping block %d because it is already marked bad. (%d total)", block, total_bad_blocks_count);
        printf("x");
        fflush(stdout);
        continue;
      }

      uint32_t page_addr = nand_block_page_to_page_addr(block, 0, g_nand_handle.chipinfo->block_addr_offset);
      status = nand_erase_block(&g_nand_handle, page_addr);

      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_read_page() error: %d for block %u, continuing anyway", status, block);
      }

      page_offset = 0;
      for (page = 0 ; page < NAND_PAGES_PER_BLOCK; page++) {

        //printf("Write to %d:%d:%d (%d B): \n", (int)block, (int)page, (int)page_offset, (int)data_size);
        status = nand_write_page(&g_nand_handle,
          block,
          page,
          page_offset, data, data_size);
        if (status != 0) {
          LOGE("nand_write_read", "nand_write_page status %d, continuing anyway", status);
        }

      }  // pages

      printf(".");
      fflush(stdout);

    }  // blocks

    // Print write speed (in bytes/s)
    {
      TickType_t t1 = xTaskGetTickCount();
      TickType_t delta_ticks = t1 - t0;
      uint32_t delta_ms = portTICK_PERIOD_MS*delta_ticks;
  //    uint32_t bytes = UFFS_TEST_SPEED_NUM_CHUNKS * UFFS_TEST_SPEED_CHUNK_SIZE;
      uint32_t bytes = data_size * NAND_PAGES_PER_BLOCK * 30;  // TODO: Use a variable for number of blocks
      float kbytes = bytes/1024.0;
      float kbytes_per_s = (float)bytes/delta_ms;
      //printf("\nRead speed: %0.2f KB/s\n", kbytes_per_s);
      printf("\n");
      printf("Write %0.1f KB at %0.3f KB/s\n", kbytes, kbytes_per_s);
    }


    t0 = xTaskGetTickCount();

    for (block = 0 ; block < 30; block++) {
      for (page = 0 ; page < NAND_PAGES_PER_BLOCK; page++) {

        status = nand_read_page(&g_nand_handle,
          block,
          page,
          page_offset, data_out, data_size);

#if 0
        // If the memory is different, count it as an error:
        if (status == 0) {
          status = memcmp(data, data_out, data_size);
          if (memcmp(data, data_out, data_size) != 0) {
            //LOGE("nand_write_read", "status==0 but memcmp result was: %d for block %d", status, block);
            printf("?");
            fflush(stdout);
          }
        }
#endif
      }

      printf(".");
      fflush(stdout);

    }  // blocks

    // Print read speed (in bytes/s)
    {
      TickType_t t1 = xTaskGetTickCount();
      TickType_t delta_ticks = t1 - t0;
      uint32_t delta_ms = portTICK_PERIOD_MS*delta_ticks;
  //    uint32_t bytes = UFFS_TEST_SPEED_NUM_CHUNKS * UFFS_TEST_SPEED_CHUNK_SIZE;
      uint32_t bytes = data_size * NAND_PAGES_PER_BLOCK * 30;  // TODO: Use a variable for number of blocks
      float kbytes = bytes/1024.0;
      float kbytes_per_s = (float)bytes/delta_ms;
      //printf("\nRead speed: %0.2f KB/s\n", kbytes_per_s);
      printf("\n");
      printf("Read %0.1f KB at %0.3f KB/s\n", kbytes, kbytes_per_s);
    }

  #ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
    free(data);
    free(data_out);
  #endif
    printf("Done. Marked %d this run, %d total", marked_bad_blocks_count, total_bad_blocks_count);

  } // run_count



}

void
nand_write_read_checkblocks_command(int argc, char **argv)
{
  int status;
  int block;
  int page = 0;
  // data_size does not include the ECC region used by the hardware, so /2:
//FIXME derek test:  uint32_t data_size = (NAND_PAGE_SIZE + (NAND_SPARE_SIZE/2));
  uint32_t data_size = (NAND_PAGE_SIZE + 2);

#ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
  uint8_t *data = malloc(NAND_PAGE_SIZE + NAND_SPARE_SIZE);
  uint8_t *data_out = malloc(NAND_PAGE_SIZE + NAND_SPARE_SIZE);

  if ((data == NULL) || (data_out == NULL)) {
    LOGE("nand_write_read", "malloc failure, returned NULL");
  }
#else
  // If these are not static, you'll need a huge Task stack size to push them.
  static uint8_t data[(NAND_PAGE_SIZE + NAND_SPARE_SIZE)];
  static uint8_t data_out[(NAND_PAGE_SIZE + NAND_SPARE_SIZE)];
#endif

  uint16_t page_offset; // write the entire page with spare
  uint32_t index;

  int run_count = 0;
  for (run_count = 0 ; run_count < 1; run_count++) {

    // Populate some non-zero values:
    for (index = 0 ; index < data_size; index++) {
        data[index] = index & 0xFF;
    }

    // ...and make the spare data count down instead of up, for easy identification
    for (index = 0 ; index < NAND_SPARE_SIZE; index++) {
        data[NAND_PAGE_SIZE + index] = (NAND_SPARE_SIZE - index) & 0xFF;
    }

    // ...but don't overwrite the is_good_mark with a non-0xFF mark:
    // Emulate a bad block marker and seal byte:
    data[NAND_PAGE_SIZE] = 0xFF;
    data[NAND_PAGE_SIZE + 1] = 0x00;

    data_size = NAND_PAGE_SIZE + NAND_SPARE_SIZE;

    // A marker value:
    for (index = 0 ; index < data_size; index++) {
        data_out[index] = 0xa5;
    }


    block = 0;
    page = 0;

    page_offset = 0;
    int total_bad_blocks_count = 0;
    int marked_bad_blocks_count = 0;

//    for (block = 0 ; block < NAND_BLOCK_COUNT; block++) {
    for (block = 0 ; block < 10; block++) {

      // First see if this is a bad block before erasing it for a write test.
      // We don't want to lose previously identified bad blocks.
      // Also, minimize prints for speed:
      //printf("Read from %d:%d:%d (%d B): \n", (int)block, (int)page, (int)page_offset, (int)data_size);
      if (block % 64 == 0)printf("\n");

      // The datasheet says to turn off ECC before checking this byte. There must
      // be some slim possibility that it will ECC "correct" it from 0x00 to 0xFF.
      status = nand_ecc_enable(&g_nand_handle, FALSE);
      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_ecc_enable(1): %d for block %u, continuing anyway", status, block);
      }

      const uint8_t is_good_mark = 0xFF;
      int is_bad_block = 1;

      page_offset = NAND_PAGE_SIZE;

      status = nand_read_page(&g_nand_handle,
        block,
        page,
        page_offset, data_out, 1);

      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_read_page() error: %d for block %u, continuing anyway", status, block);
      }

      status = nand_ecc_enable(&g_nand_handle, TRUE);   // Turn it back on
      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_ecc_enable(2) error: %d for block %u, continuing anyway", status, block);
      }

      // 0xFF is the code indicating the block is good (originally set at the factory).
      if (data_out[0] == is_good_mark) {
        is_bad_block = 0;  // It's good
      } else {
        is_bad_block = 1;  // It's bad
      }

      if (is_bad_block) {
        total_bad_blocks_count++;
        //LOGE(TAG, "IsBadBlock: Skipping block %d because it is already marked bad. (%d total)", block, total_bad_blocks_count);
        printf("x");
        fflush(stdout);
        continue;
      }

      uint32_t page_addr = nand_block_page_to_page_addr(block, 0, g_nand_handle.chipinfo->block_addr_offset);
      status = nand_erase_block(&g_nand_handle, page_addr);

      if (status < 0) {
        LOGE(TAG, "IsBadBlock: nand_read_page() error: %d for block %u, continuing anyway", status, block);
      }

      page_offset = 0;
      for (page = 0 ; page < NAND_PAGES_PER_BLOCK; page++) {

        //printf("Write to %d:%d:%d (%d B): \n", (int)block, (int)page, (int)page_offset, (int)data_size);
        status = nand_write_page(&g_nand_handle,
          block,
          page,
          page_offset, data, data_size);
        if (status != 0) {
          LOGE("nand_write_read", "nand_write_page status %d, continuing anyway", status);
        }

        //printf("Read from %d:%d:%d (%d B): \n", (int)block, (int)page, (int)page_offset, (int)data_size);
        status = nand_read_page(&g_nand_handle,
          block,
          page,
          page_offset, data_out, data_size);

        // If the memory is different, count it as an error:
        if (status == 0) {
          status = memcmp(data, data_out, data_size);
          if (memcmp(data, data_out, data_size) != 0) {
            //LOGE("nand_write_read", "status==0 but memcmp result was: %d for block %d", status, block);
            printf("?");
            fflush(stdout);
          }
        }

        // Must be checked for ==0, not <0, to catch positive ECC corrections
        if (status != 0) {

          total_bad_blocks_count++;
          marked_bad_blocks_count++;

          printf("X");
          fflush(stdout);


          //LOGE("nand_write_read", "nand_read_page status %d. Marking bad block: %d (%d this run, %d total)", status, block, marked_bad_blocks_count, total_bad_blocks_count);

          uint8_t is_bad_marker = 0x00;
          // the first page, page 0
          page = 0;

          // Erase block before writing the is_bad_marker to page 0 at NAND_PAGE_SIZE.
          uint32_t page_addr = nand_block_page_to_page_addr(block, 0, g_nand_handle.chipinfo->block_addr_offset);
          status = nand_erase_block(&g_nand_handle, page_addr);
          if (status < 0) {
            LOGE(TAG, "MarkBadBlock: %d.%d: "
              "nand_erase_block() error: %d, continuing to next block", (int)block, (int)page, status);
          }

          status = nand_write_page(&g_nand_handle, block, page,
            NAND_PAGE_SIZE, &is_bad_marker, sizeof(is_bad_marker));
          if (status < 0) {
            LOGE(TAG, "MarkBadBlock: %d.%d marker: "
              "nand_write_page() error: %d, continuing to next block", (int)block, (int)page, status);
          }

          break;  // Go to the next block.

        }
        //print_buf(data_out, data_size);

      }  // pages

      // Success, did not break out, good block for all pages:
      printf(".");
      fflush(stdout);

    }  // blocks

  #ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
    free(data);
    free(data_out);
  #endif
    printf("Done. Marked %d this run, %d total", marked_bad_blocks_count, total_bad_blocks_count);

  } // run_count
}

void
nand_safe_wipe_all_blocks_command(int argc, char **argv)
{
  // WARN: This might erase knowledge of bad blocks from the factory! See code below
  int status;
  int block;
  int page = 0;

  uint16_t page_offset;
  uint8_t data;

  nand_unlock(&g_nand_handle);

  for (block = 0; block < NAND_BLOCK_COUNT; block++) {

    if (block % 64 == 0)printf("\n");

    // First check if it is a Bad Block:
    status = nand_ecc_enable(&g_nand_handle, FALSE);
    if (status != 0) {
      LOGE("nand_safe_wipe", "nand_ecc_enable(1): %d for block %d, continuing anyway", status, block);
    }

    page_offset = NAND_PAGE_SIZE;  // Go to the end of the page

    uint8_t is_good_mark = 0xFF;
    data = 0x69;
    //int is_bad_block = -1;

    status = nand_read_page(&g_nand_handle,
      block,
      page,
      page_offset, &data, sizeof(data));

    if (status < 0) {
      LOGE("nand_safe_wipe", "nand_read_page status %d, continuing anyway", status);
    }

    status = nand_ecc_enable(&g_nand_handle, TRUE);
    if (status != 0) {
      LOGE("nand_safe_wipe", "nand_ecc_enable(2): %d for block %d, continuing anyway", status, block);
    }

    // 0xFF is the code indicating the block is good (originally set at the factory).
//    if (0) {  // WARN: Force erase!
    if (data != is_good_mark) {
      //is_bad_block = 0;  // It's good
      //printf("Bad block found: %d, got marker: 0x%x. Skipping...\n", block, data);
      printf("x");
      fflush(stdout);
    } else {
      //LOGI("nand_safe_wipe", "marker: got 0x%x for block %d, continuing anyway", data, block);
      //is_bad_block = 1;  // It's bad
      uint32_t page_addr = nand_block_page_to_page_addr(block, 0, g_nand_handle.chipinfo->block_addr_offset);
      status = nand_erase_block(&g_nand_handle, page_addr);
      if (status != 0) {
        LOGE("nand_safe_wipe", "nand_erase_block status %d, continuing anyway", status);
      }
      printf(".");
      fflush(stdout);

    }
  }

  printf("Done.\n");
}


void
nand_read_all_blocks_command(int argc, char **argv)
{
  // WARN: This erases knowledge of bad blocks from the factory!
  int status;
  int block;
  int page = 0;

  uint16_t page_offset;
  const uint32_t data_size = (NAND_PAGE_SIZE + NAND_SPARE_SIZE);
#ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
  uint8_t *data = malloc(data_size);

  if (data == NULL) {
    LOGE("nand_read_all_blocks", "malloc failure, returned NULL");
  }
#else
  // If these are not static, you'll need a huge Task stack size to push them.
  static uint8_t data[data_size];
#endif
  uint8_t marker;

//  for (block = 0 ; block < NAND_BLOCK_COUNT; block++) {
  for (block = 0 ; block < 20; block++) {

    page_offset = 0;  // Read the entire page

    status = nand_read_page(&g_nand_handle,
      block,
      page,
      page_offset, data, sizeof(data));

    if (status != 0) {
      LOGE("nand_read_all", "nand_read_page status %d, continuing anyway", status);
    }

    // Now check if it is a Bad Block (nand_read_page should already do this):

    status = nand_ecc_enable(&g_nand_handle, FALSE);
    if (status != 0) {
      LOGE("nand_read_all", "nand_ecc_enable(1): %d for block %d, continuing anyway", status, block);
    }

    page_offset = NAND_PAGE_SIZE;  // Go to the end of the page

    uint8_t is_good_marker = 0xFF;
    marker = 0x69;
    //int is_bad_block = 1;

    status = nand_read_page(&g_nand_handle,
      block,
      page,
      page_offset, &marker, sizeof(marker));

    if (status != 0) {
      LOGE("nand_read_all", "nand_read_page status %d, continuing anyway", status);
    }

    status = nand_ecc_enable(&g_nand_handle, TRUE);
    if (status != 0) {
      LOGE("nand_read_all", "nand_ecc_enable(2): %d for block %d, continuing anyway", status, block);
    }

    // 0xFF is the code indicating the block is good (originally set at the factory).
    if (marker == is_good_marker) {
      //is_bad_block = 0;  // It's good
    } else {
      LOGI("nand_read_all", "marker: got 0x%x for block %d, continuing anyway", marker, block);
      //is_bad_block = 1;  // It's bad
    }

    printf("Read block: %d, got marker: 0x%x\n", block, marker);
    fflush(stdout);
  }
#ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
  free(data);
#endif
  printf("Done.\n");
}

void
nand_copy_pages_command(int argc, char **argv)
{
  // WARN: This erases knowledge of bad blocks from the factory!
  int status;
  int block;
  int page = 0;

  const uint32_t data_size = (NAND_PAGE_SIZE + NAND_SPARE_SIZE);
#ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
  uint8_t *data = malloc(data_size);

  if (data == NULL) {
    LOGE("nand_read_all_blocks", "malloc failure, returned NULL");
  }
#else
  // If these are not static, you'll need a huge Task stack size to push them.
  static uint8_t data[data_size];
#endif

  uint32_t src_page_addr, dest_page_addr;

  for (block = 1 ; block < 5; block++) {
    for (page = 0 ; page < 10; page++) {

      // Move each page to the next block:
      printf("Copying %d.%d to %d.%d\n", block, page, block+1, page);
      fflush(stdout);

      src_page_addr = nand_block_page_to_page_addr(block, page, NAND_BLOCK_ADDR_OFFSET);
      dest_page_addr = nand_block_page_to_page_addr(block+1, page, NAND_BLOCK_ADDR_OFFSET);

      status = nand_copy_page_from_cache(
        &g_nand_handle,
        src_page_addr,
        dest_page_addr);
      if (status != 0) {
        LOGE("nand_copy_pages", "status %d for copy %d.%d to %d.%d, continuing anyway", status, block, page, block+1, page);
      }

      // Move each page to the next page in this block:
      printf("Copying %d.%d to %d.%d\n", block, page, block, page+1);
      fflush(stdout);

      src_page_addr = nand_block_page_to_page_addr(block, page, NAND_BLOCK_ADDR_OFFSET);
      dest_page_addr = nand_block_page_to_page_addr(block+1, page, NAND_BLOCK_ADDR_OFFSET);

      status = nand_copy_page_from_cache(
        &g_nand_handle,
        src_page_addr,
        dest_page_addr);
      if (status != 0) {
        LOGE("nand_copy_pages", "status %d for copy %d.%d to %d.%d, continuing anyway", status, block, page, block+1, page);
      }

      fflush(stdout);
    }
  }
#ifdef NAND_TESTS_USE_MALLOC  // use static ram if your heap is low
  free(data);
#endif
  printf("Done.\n");
}
