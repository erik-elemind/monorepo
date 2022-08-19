/*
 * COBSR_RLE0.h
 *
 *  Created on: Apr 11, 2022
 *      Author: DavidWang
 */

#ifndef COMPRESSION_NEW_COBSR_RLE0_H_
#define COMPRESSION_NEW_COBSR_RLE0_H_


#include <stddef.h>
#include <stdint.h>
#include "COBSR.h"

#ifdef __cplusplus
extern "C" {
#endif


/* When more than (OVER_COUNT_DELIMETER-DEFAULT_COUNT_VAL) zeros or non-zeros are sequentially counted,
 * the count code 'OVER_COUNT_DELIMETER' is recorded in the encoded stream.*/
#define OVER_COUNT_DELIMETER 0xFF

/* This encoding strategy uses 2 counters, a counter for zeros and a counter for non-zeros.
 * The following value is the starting value for all counters, meaning if there have been 0
 * elements counted, the value is 'DEFAULT_COUNT_VAL'. */
#define DEFAULT_COUNT_VAL 1


/*****************************************************************************
 * Function prototypes
 ****************************************************************************/


cobsr_encode_result cobsr_rle0_encode(uint8_t* dst_buf_ptr, const int dst_buf_len,
    const uint8_t* src_ptr, const int src_len);
cobsr_decode_result cobsr_rle0_decode(uint8_t* dst_buf_ptr, const int dst_buf_len,
    const uint8_t* src_ptr, const int src_len);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* COMPRESSION_NEW_COBSR_RLE0_H_ */
