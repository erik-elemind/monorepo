/*
 * dhara_metadata_cache.h
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Dhara metadata cache to minimize NAND page reads.
 *
 */

#include <limits.h>
#include <string.h>

#include "dhara_metadata_cache.h"
#include "dhara_nand.h"

typedef struct {
  uint32_t page_addr;
  uint16_t page_offset;

  unsigned int last_hit; 

  uint8_t buffer[DHARA_META_SIZE];   // 132 Bytes
} dhara_metadata_cache_entry_t;

// The entry count is set to 11 because Dhara walks that many nodes to
// find a dhara_page_t page_addr from the desired sector number.
#define DHARA_METADATA_CACHE_ENTRY_COUNT 11
static dhara_metadata_cache_entry_t g_cache[DHARA_METADATA_CACHE_ENTRY_COUNT];

static unsigned int g_hit_counter = 0;   // Used to track least recently used

void dhara_metadata_cache_init(void)
{
  for (int index = 0; index < DHARA_METADATA_CACHE_ENTRY_COUNT; index++) {
    dhara_metadata_cache_entry_t *entry = &(g_cache[index]);

    entry->page_addr = DHARA_PAGE_NONE;
    entry->last_hit = 0;
  }
}

void dhara_metadata_cache_invalidate(dhara_page_t page, dhara_page_t mask)
{
  for (int index = 0; index < DHARA_METADATA_CACHE_ENTRY_COUNT; index++) {
    dhara_metadata_cache_entry_t *entry = &(g_cache[index]);

    if (!((entry->page_addr ^ page) & ~mask)) {
      entry->page_addr = DHARA_PAGE_NONE;
    }
  }
}

static int get_least_recently_used_cache_index(void)
{
  unsigned int least_used_hit_number = UINT_MAX;
  int least_used_entry_index = 0;

  // Brute-force search. It's only 11 entries.
  for (int index = 0; index < DHARA_METADATA_CACHE_ENTRY_COUNT; index++) {
    dhara_metadata_cache_entry_t *entry = &(g_cache[index]);
    if (entry->last_hit < least_used_hit_number) {
      least_used_entry_index = index;  // The candidate
      least_used_hit_number = entry->last_hit;
    }
  }

  return least_used_entry_index;
}

// Called with hit_count wraps. Simplistically resets all values.
static void reset_all_hit_counts_to_zero(void)
{
  for (int index = 0; index < DHARA_METADATA_CACHE_ENTRY_COUNT; index++) {
    g_cache[index].last_hit = 0;
  }
}

static void increment_hit_counter(void)
{
  // We must deal with g_hit_counter wrapping back to zero.
  g_hit_counter++;
  if (g_hit_counter == UINT32_MAX) {
    reset_all_hit_counts_to_zero();
    g_hit_counter++; // Make the next entry more recent than zero
  }
}

int dhara_metadata_cache_get(uint32_t page_addr, uint16_t page_offset, uint8_t *buffer)
{
  int index;

  // Brute-force search. It's only 11 entries.  
  for (index = 0; index < DHARA_METADATA_CACHE_ENTRY_COUNT; index++) {
    dhara_metadata_cache_entry_t *entry = &(g_cache[index]);
    if ((entry->page_addr == page_addr) && (entry->page_offset == page_offset)) {
      // Found it!
      memcpy(buffer, entry->buffer, DHARA_META_SIZE);

      // Make this entry the most recently used.
      increment_hit_counter();
      entry->last_hit = g_hit_counter;

      return 1;
    }
  }

  return 0;  // Not in the g_cache.
}

void dhara_metadata_cache_set(uint32_t page_addr, uint16_t page_offset, uint8_t *buffer)
{
  // Replace the least recently used entry.
  int index = get_least_recently_used_cache_index();
  dhara_metadata_cache_entry_t *entry = &(g_cache[index]);

  entry->page_offset = page_offset;
  entry->page_addr = page_addr;
  
  increment_hit_counter();
  entry->last_hit = g_hit_counter;

  memcpy(entry->buffer, buffer, DHARA_META_SIZE);
}


