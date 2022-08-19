/*
 * bit_copy.h
 *
 *  Created on: Jan 2, 2019
 *      Author: david
 */

#ifndef _BIT_COPY_H_
#define _BIT_COPY_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BITMASKU8(size) ((1U << (size)) - 1)
#define BITMASKU8_RANGE(start,size) (BITMASKU8(size) << (start))

void copyBitsHelper(uint8_t *dst, const uint8_t *src,
    uint8_t numBits, uint8_t dstBitOff, uint8_t srcBitOff);
void copyBitsLittleEndian(uint8_t* dst, const uint8_t* src,
        size_t numBits, size_t dstTotBitOff, size_t srcTotBitOff);
void copyBitsBigEndian(uint8_t* dst, const uint8_t* src,
        size_t numBits,
        size_t dstTotBitOff, size_t srcTotBitOff,
        size_t dstNumBytes, size_t srcNumBytes);

void reverse(uint8_t *start, int size);
bool isEqual(uint8_t* bytes1, uint8_t* bytes2, size_t size);


#endif // _BIT_COPY_H_
