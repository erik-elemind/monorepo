/*
 * QuickSelect.h
 *
 *  Created on: Apr 11, 2022
 *      Author: DavidWang
 */

#ifndef COMPRESSION_NEW_QUICKSELECT_H_
#define COMPRESSION_NEW_QUICKSELECT_H_

#include "compression_config.h"

#ifdef __cplusplus
extern "C" {
#endif

CMPR_FLOAT select7MO3_ascending(CMPR_FLOAT* array, const int length, const int kTHvalue);

CMPR_FLOAT select7MO3_descending(CMPR_FLOAT* array, const int length, const int kTHvalue);

CMPR_FLOAT FloydWirth_kth_ascending(CMPR_FLOAT* arr, const int length, const int kTHvalue);

CMPR_FLOAT FloydWirth_kth_descending(CMPR_FLOAT* arr, const int length, const int kTHvalue);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* COMPRESSION_NEW_QUICKSELECT_H_ */
