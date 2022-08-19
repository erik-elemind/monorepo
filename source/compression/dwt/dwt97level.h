/*
 * dwt97level.h
 *
 *  Created on: Mar 13, 2022
 *      Author: DavidWang
 */

#ifndef COMPRESSION_DWT_DWT97LEVEL_H_
#define COMPRESSION_DWT_DWT97LEVEL_H_

#include "../compression_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void fwt97level(CMPR_FLOAT* x,int n,int level);
void iwt97level(CMPR_FLOAT* x,int n,int level);

#ifdef __cplusplus
}
#endif


#endif /* COMPRESSION_DWT_DWT97LEVEL_H_ */
