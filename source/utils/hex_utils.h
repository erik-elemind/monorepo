/*
 * hex_utils.h
 *
 *  Created on: Apr 25, 2022
 *      Author: DavidWang
 */

#ifndef UTILS_HEX_UTILS_H_
#define UTILS_HEX_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

int HexStringToBytes(const char *hexStr,
    unsigned char *buffer,
    unsigned int size,
    unsigned int *outputLen);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_HEX_UTILS_H_ */
