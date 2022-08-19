/*
 * fast_math.h
 *
 *  Created on: Aug 16, 2021
 *      Author: DavidWang
 */

#ifndef UTILS_FAST_MATH_H_
#define UTILS_FAST_MATH_H_


#ifdef __cplusplus
extern "C" {
#endif

float log2f_approx(float X);

// copied from: http://openaudio.blogspot.com/2017/02/faster-log10-and-pow.html
//powf(10.f,x) is exactly exp(log(10.0f)*x)
#define pow10f_fast(x) expf(2.302585092994046f*x)

//log10f is exactly log2(x)/log2(10.0f)
#define log10f_fast(x)  (log2f_approx(x)*0.3010299956639812f)


#ifdef __cplusplus
}
#endif


#endif /* UTILS_FAST_MATH_H_ */
