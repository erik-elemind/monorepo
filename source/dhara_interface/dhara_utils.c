/*
 * dhara_utils.h
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Dhara Flash Translation Layer utilities (buffer, mount, etc.)
 *
 */

#include <stdatomic.h>
#include "nand.h"
#include "loglevels.h"

#include "dhara_utils.h"
#include "dhara_metadata_cache.h"

#define DHARA_GC_RATIO 4

static uint8_t layout_buffer[NAND_PAGE_PLUS_SPARE_SIZE];

static uint8_t dhara_page_buffer[NAND_PAGE_SIZE];

// Create user data struct
static nand_user_data_t g_nand_handle = {
  .chipinfo = &nand_chipinfo,
  .layout_buffer = layout_buffer,
  .layout_size_bytes = sizeof(layout_buffer),
  .layout_page_addr = DHARA_PAGE_NONE
};

// Map info (uses log(base2) page size as bitshift position and count)

const struct dhara_nand g_dhara_nand = {
  .user_data = &g_nand_handle,

  .log2_page_size = NAND_PAGE_SIZE_LOG2,
  .log2_ppb = NAND_PAGES_PER_BLOCK_LOG2,
  .num_blocks = NAND_BLOCK_COUNT
};

/// Logging prefix
static const char* TAG = "dhara_utils";

static struct dhara_map g_dhara_map;

void
dhara_pretask_init(void)
{
  dhara_error_t err;
  int status;

  //
  // NAND Flash Init:
  //
  nand_user_data_t *user_data = g_dhara_nand.user_data;

  // Data Sheet: Wait 5 ms before SPI comms is allowed.
  nand_platform_yield_delay(6);

  status = nand_unlock(user_data);
  if (status != NAND_NO_ERR) {
    LOGE(TAG, "dhara_pretask_init: nand_unlock() error: %d", status);
    ; // continue anyway...
  }

  // Turn on flash ECC handling (btw it's on by default after power-up):
  status = nand_ecc_enable(user_data, true);
  if (status != NAND_NO_ERR) {
    LOGE(TAG, "dhara_pretask_init: nand_ecc_enable() error: %d", status);
    ; // continue anyway...
  }

  //
  // Dhara Flash Translation Layer (FTL) Map Init:
  //
  dhara_metadata_cache_init();
  dhara_map_init(&g_dhara_map, &g_dhara_nand, (uint8_t *)dhara_page_buffer, DHARA_GC_RATIO);

  status = dhara_map_resume(&g_dhara_map, &err);
  if (status == -1) {
    LOGI(TAG, "dhara_map_resume: no journal found (err: %d); initialized an empty map", (int)err);

    // Save this new map:
    status = dhara_map_sync(&g_dhara_map, &err);
    if (status != 0) {
      LOGE(TAG, "dhara_map_sync: %d (err: %d)", status, (int)err);
    }
    
  } else {
    LOGI(TAG, "dhara_map_resume: success");
  }

}

struct dhara_map *dhara_get_my_map(void)
{
  return &g_dhara_map;
}

uint16_t dhara_map_sector_size_bytes(struct dhara_map *map)
{
  return (1 << map->journal.nand->log2_page_size);
}

// It would be nice to make the recovery hint an extern and inline these
// functions, but mixing C11 atomics with C++ code is not so easy
static atomic_int dhara__recovery_hint;

void dhara_recovery_hint_set(void)
{
  atomic_store_explicit(&dhara__recovery_hint, 1, memory_order_release);
}

int dhara_recovery_hint_take(void)
{
  if (!atomic_load_explicit(&dhara__recovery_hint, memory_order_acquire)) {
    return 0;
  }

  // A lost update is possible here, but ok -- we're already
  // returning 1, and multiple calls to set() that happen before
  // take() shouldn't change the behaviour.
  atomic_store_explicit(&dhara__recovery_hint, 0, memory_order_release);
  return 1;
}
