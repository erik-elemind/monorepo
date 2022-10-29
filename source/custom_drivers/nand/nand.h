/*
 * nand.h
 *
 *  Created on: Oct 28, 2022
 *      Author: DavidWang
 */

#ifndef CUSTOM_DRIVERS_NAND_NAND_H_
#define CUSTOM_DRIVERS_NAND_NAND_H_

#include "config.h"

// System specific NAND header
#if (defined(USE_NAND_GD5F4GQ4_4KPAGE) && (USE_FLASH_4KPAGE_GD5F4GQ4 > 0U))
#include "nand_GD5F4GQX.h"
#elif (defined(USE_NAND_GD5F4GQ6_2KPAGE) && (USE_NAND_GD5F4GQ6_2KPAGE > 0U))
#include "nand_GD5F4GQX.h"
#elif (defined(USE_NAND_W25N04KW) && (USE_NAND_W25N04KW > 0U))
#include "nand_W25N04KW.h"
#else
#error Must define type of NAND
#endif

#endif /* CUSTOM_DRIVERS_NAND_NAND_H_ */
