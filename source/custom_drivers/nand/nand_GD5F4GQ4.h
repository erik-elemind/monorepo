/*
 * nand_gd5f4gq4.h
 *
 * Copyright (C) 2021 Igor Institute Inc.
 *
 * Created: Feb 2021
 * Author:  Derek Simkowiak
 *
 * Description: NAND configuration for Gigadevice GD5F4GQ4 family (4 KB page size)
*/
#ifndef NAND_GD5F4GQ4_H
#define NAND_GD5F4GQ4_H

#include "stdint.h"

/** Number of blocks */
#define NAND_BLOCK_COUNT 0x800  // 2048 blocks

/** Max page address (128K pages == 0x000000-0x01FFFF) */
#define NAND_PAGE_ADDR_MAX 0x01FFFF

/** Number of pages per block. */
#define NAND_PAGES_PER_BLOCK 64

/** Number of pages per block log2. */
#define NAND_PAGES_PER_BLOCK_LOG2 6

/** Page size (without "spare" area). */
#define NAND_PAGE_SIZE 0x1000  // 4096 B

/** Page size log2 (without "spare" area). */
#define NAND_PAGE_SIZE_LOG2 12  // (1<<12 == 4096)

/** Spare area size. */
// NOTE: With hardware ECC turned on, the last 128 bytes of the 256 B 
// "spare area" are reserved for ECC data. This #define should only report 
// the usable bytes. So set 128 if using hardware ECC.
#define NAND_SPARE_SIZE 0x80  // Report 128 B of the 256 B "spare", because hardware ECC

/** Block size (without "spare" areas). */
#define NAND_BLOCK_SIZE (NAND_PAGES_PER_BLOCK * NAND_PAGE_SIZE)

/** Complete page size, incl. spare region: */
#define NAND_PAGE_PLUS_SPARE_SIZE (NAND_PAGE_SIZE + NAND_SPARE_SIZE)

/** Block address mask. */
#define NAND_BLOCK_ADDR_MASK 0x01FFC0

/** Block address offset (in bits). */
#define NAND_BLOCK_ADDR_OFFSET 6

/** Page address mask (no offset). */
#define NAND_PAGE_ADDR_MASK 0x00003F

/** Number of bits used for ECC (in the status register)*/
#define NAND_ECC_STATUS_REG_BIT_COUNT 3

/** Maximum number of bits allowed before moving data and marking this block as bad: */
//
// This says how many bit flips to ignore. More ECC bits than this will trigger
// the UFFS file system to move the entire block to a fresh block, and will
// mark this block as a bad block:
#define NAND_ECC_SAFE_CORRECTED_BIT_COUNT 4

/** Number of bit errors that can be corrected by this NAND chip */
#define NAND_ECC_MAX_CORRECTED_BIT_COUNT 8

/** Returns the number of bits corrected given the status register*/
inline static uint32_t
ecc_bits_map(uint8_t status_register_byte)
{
  uint32_t ecc_bits_corrected;
  // Get only ECC bits from status register
  uint8_t ecc_bits = (status_register_byte >> 4) & 0x07;

  // Assume GigaDevices-style mapping of bits to count (starts w/3):
  switch (ecc_bits) {
    case 0x0:
      ecc_bits_corrected = 0;
      break;
    case 0x1:
      ecc_bits_corrected = 3; // 3 or less, so just assume 3
      break;
    case 0x2:
      ecc_bits_corrected = 4;
      break;
    case 0x3:
      ecc_bits_corrected = 5;
      break;
    case 0x4:
      ecc_bits_corrected = 6;
      break;
    case 0x5:
      ecc_bits_corrected = 7;
      break;
    case 0x6:
      ecc_bits_corrected = 8;
      break;
    default:
      ecc_bits_corrected = 9;  // 9 or more, too many to correct
      break;
  }
  return ecc_bits_corrected;
}

#endif  // NAND_GD5F4GQ4_H
