/*
 * dwt97.h
 *
 *  Created on: Mar 13, 2022
 *      Author: DavidWang
 */

#ifndef COMPRESSION_DWT_DWT97_H_
#define COMPRESSION_DWT_DWT97_H_

#include "compression_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  fwt97 - Forward biorthogonal 9/7 wavelet transform (lifting implementation)
 *
 *  x is an input signal, which will be replaced by its output transform.
 *  n is the length of the signal, and must be a power of 2.
 *
 *  The first half part of the output signal contains the approximation coefficients.
 *  The second half part contains the detail coefficients (aka. the wavelets coefficients).
 *
 *  See also iwt97.
 */
void fwt97(CMPR_FLOAT* x,int n);


/**
 *  iwt97 - Inverse biorthogonal 9/7 wavelet transform
 *
 *  This is the inverse of fwt97 so that iwt97(fwt97(x,n),n)=x for every signal x of length n.
 *
 *  See also fwt97.
 */
void iwt97(CMPR_FLOAT* x,int n);

#ifdef __cplusplus
}
#endif

#endif /* COMPRESSION_DWT_DWT97_H_ */
