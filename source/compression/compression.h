/*
 * Compression.h
 *
 *  Created on: Apr 11, 2022
 *      Author: DavidWang
 */

#ifndef COMPRESSION_NEW_COMPRESSION_H_
#define COMPRESSION_NEW_COMPRESSION_H_

#include <stdint.h>
#include "compression_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    uint8_t   compression_version; // unsigned 8  bit integer
    uint32_t  frame_size;          // unsigned 32 bit integer
    uint32_t  keep_num_coeff;      // unsigned 32 bit integer
    uint8_t   q_bits;              // unsigned 8  bit integer
    char      wavelet_name[255];   // up to 255 characters (plus 1 byte counter)
    uint8_t   wavelet_level;       // unsigned 8  bit integer
} comp_params_t;

#define COMP_HEADER_MAX_SIZE (sizeof(comp_params_t)+1) // plus 1 needed to write number of chars

int compress_header(comp_params_t *cp, uint8_t* comp_hdr, size_t comp_hdr_size);

void decompress_header(uint8_t* comp_hdr, size_t comp_hdr_size, comp_params_t *cp);

// used to be: "int* frame_buf"
int compress_frame(CMPR_FLOAT* frame_buf, const int frame_size, const int wavelet_level, const int q_bits, const int keep_num_coeff, uint8_t* comp_buf, int comp_buf_size);

void decompress_frame(uint8_t* comp_buf, const int comp_size, const int wavelet_level, const int q_bits, uint8_t* frame_buf, const int frame_size);

CMPR_FLOAT prd(int* orig, int orig_size, int* comp, int comp_size);


#ifdef __cplusplus
}
#endif

#endif /* COMPRESSION_NEW_COMPRESSION_H_ */
