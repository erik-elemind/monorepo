/*
 * nand_W25N04KW_config.h
 *
 * Copyright (C) 2021 Igor Institute; Inc.
 *
 * Created: Feb 2021
 * Author:  Derek Simkowiak
 *
 * Description: NAND configuration for Gigadevice GD5F4GQ6 family (2 KB page size)
*/
#ifndef NAND_GD5F4GQ6_H
#define NAND_GD5F4GQ6_H

#include "stdint.h"

/** Number of blocks */
#define NAND_BLOCK_COUNT 0x1000  // 4096 blocks

/** Max page address (256K pages == 0x000000-0x03FFFF) */
#define NAND_PAGE_ADDR_MAX 0x03FFFF // (12 bit block address) + (6 bit page address) = 18 bits

/** Number of pages per block. */
#define NAND_PAGES_PER_BLOCK 64

/** Number of pages per block log2. */
#define NAND_PAGES_PER_BLOCK_LOG2 6

/** Page size (without "spare" area). */
#define NAND_PAGE_SIZE 0x800  // 2048 B

/** Page size log2 (without "spare" area). */
#define NAND_PAGE_SIZE_LOG2 11  // (1<<11 == 2048)

/** Spare area size. */
#define NAND_SPARE_SIZE 0x40  // Report 64 B of the 128 B "spare", because hardware ECC

/** Block size (without "spare" areas). */
#define NAND_BLOCK_SIZE (NAND_PAGES_PER_BLOCK * NAND_PAGE_SIZE)

/** Complete page size, incl. spare region: */
#define NAND_PAGE_PLUS_SPARE_SIZE (NAND_PAGE_SIZE + NAND_SPARE_SIZE)

/** Block address mask. */
#define NAND_BLOCK_ADDR_MASK 0x03FFC0

/** Block address offset (in bits). */
#define NAND_BLOCK_ADDR_OFFSET 6

/** Page address mask (no offset). */
#define NAND_PAGE_ADDR_MASK 0x00003F

/** Maximum number of bits allowed before moving data and marking this block as bad: */
//
// This says how many bit flips to ignore. More ECC bits than this will trigger
// the UFFS file system to move the entire block to a fresh block, and will
// mark this block as a bad block:
#define NAND_ECC_SAFE_CORRECTED_BIT_COUNT 4

/** Number of bit errors that can be corrected by this NAND chip */
#define NAND_ECC_MAX_CORRECTED_BIT_COUNT 7

#endif  // NAND_GD5F4GQ6_H
