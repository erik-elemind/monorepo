/*
 * dwt53.h
 *
 *  Created on: Mar 13, 2022
 *      Author: DavidWang
 */

#ifndef COMPRESSION_DWT_DWT53_H_
#define COMPRESSION_DWT_DWT53_H_

#include "compression_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  fwt53 - Forward biorthogonal 5/3 wavelet transform (lifting implementation)
 *
 *  x is an input signal, which will be replaced by its output transform.
 *  n is the length of the signal, and must be a power of 2.
 *
 *  The first half part of the output signal contains the approximation coefficients.
 *  The second half part contains the detail coefficients (aka. the wavelets coefficients).
 *
 *  See also iwt53.
 */
void fwt53(double* x,int n);

/**
 *  iwt53 - Inverse biorthogonal 5/3 wavelet transform
 *
 *  This is the inverse of fwt53 so that iwt53(fwt53(x,n),n)=x for every signal x of length n.
 *
 *  See also fwt53.
 */
void iwt53(double* x,int n);

#ifdef __cplusplus
}
#endif

#endif /* COMPRESSION_DWT_DWT53_H_ */
