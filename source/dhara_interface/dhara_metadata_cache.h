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
#ifndef DHARA_METADATA_CACHE_H
#define DHARA_METADATA_CACHE_H

#include "map.h"

// Initialize by marking all entries as invalid
void dhara_metadata_cache_init(void);

// Fill buffer with cached metadata
// The passed in buffer must be of size DHARA_META_SIZE. There is no bounds checking.
// Returns 1 if page_addr.page_offset was found, 0 if not found.
int dhara_metadata_cache_get(dhara_page_t page_addr, uint16_t page_offset, uint8_t *buffer);

// Fill cache with metadata
void dhara_metadata_cache_set(dhara_page_t page_addr, uint16_t page_offset, uint8_t *buffer);

// Invalidate entries in the metadata cache matching the given page
// address. The mask argument specifies bits to be ignored (those which
// are set in the mask) when making the comparison
void dhara_metadata_cache_invalidate(dhara_page_t page, dhara_page_t mask);

#endif  // DHARA_METADATA_CACHE_H
