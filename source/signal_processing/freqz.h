/*
 * freqz.h
 *
 *  Created on: Mar 2, 2019
 *      Author: David Wang
 */

#ifndef _FREQZ_H_
#define _FREQZ_H_

#include <math.h>


template<typename T>
void freqz(T* b, T* a, size_t len, T* f, size_t flen, T Fs, T* ffr, T* ffi){
  // Computes the filter fourier coefficients for frequencies provided in array "f".
  // b - filter coefficients
  // a - filter coefficients
  // len - the length of the filter coefficients array, minus 1. (i.e. the polynomial degree).
  // f - array of frequencies for which to calculate the fourier coefficients
  // flen - the length of the frequency array
  // Fs - the sampling frequency.
  // NFFT - the number of FFT values used
  // ffr, ffi - the returned real and imaginary components of the filter fourier coefficients.
  //            the allocated size of these arrays should be the same as "flen".
  for(size_t n=0; n<flen; n++){
    T w = 2*M_PI*f[n]/Fs;
    T bfr = b[len], bfi = 0;
    T afr = a[len], afi = 0;
    // Y = P(0)*X^N + P(1)*X^(N-1) + ... + P(N-1)*X + P(N)
    for(size_t l=0; l<len; l++){
      T dfreqr = cos(w*(len-l));
      T dfreqi = sin(w*(len-l));
      bfr += b[l]*dfreqr;
      bfi += b[l]*dfreqi;
      afr += a[l]*dfreqr;
      afi += a[l]*dfreqi;
    }
    T den = afr*afr + afi*afi;
    ffr[n] = (bfr*afr+bfi*afi)/den;
    ffi[n] = (bfi*afr-bfr*afi)/den;
  }
}


template<typename T>
void freqz(T* b, T* a, size_t len, size_t NFFT, T* ffr, T* ffi){
  // Computes the filter fourier coefficients for frequencies ranging from dc to nyquist, inclusive.
  // b - filter coefficients
  // a - filter coefficients
  // len - the length of the filter coefficients array, minus 1. (i.e. the polynomial degree).
  // NFFT - the number of FFT values used
  // ffr, ffi - the returned real and imaginary components of the filter fourier coefficients.
  //            the allocated size of these arrays should be "NFFT/2+1".
  size_t HALF_NFFT = NFFT/2;
  for(size_t n=0; n<=HALF_NFFT; n++){
    T w = M_PI*n/HALF_NFFT;
    T bfr = b[len], bfi = 0;
    T afr = a[len], afi = 0;
    // Y = P(0)*X^N + P(1)*X^(N-1) + ... + P(N-1)*X + P(N)
    for(size_t l=0; l<len; l++){
      T dfreqr = cos(w*(len-l));
      T dfreqi = sin(w*(len-l));
      bfr += b[l]*dfreqr;
      bfi += b[l]*dfreqi;
      afr += a[l]*dfreqr;
      afi += a[l]*dfreqi;
    }
    T den = afr*afr + afi*afi;
    ffr[n] = (bfr*afr+bfi*afi)/den;
    ffi[n] = (bfi*afr-bfr*afi)/den;
  }
}


#endif /* _FREQZ_H_ */
