/*
 * hex_utils.c
 *
 *  Created on: Apr 25, 2022
 *      Author: DavidWang
 */

#include "hex_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


/*
 * The following function was copied from:
 * https://gist.github.com/xsleonard/7341172
 * Code Posted By: guigarma commented on Apr 18, 2021
 * Copied On: April 25th, 2022
 *
 */
int HexStringToBytes(const char *hexStr,
                     unsigned char *buffer,
                     unsigned int size,
                     unsigned int *outputLen) {
  size_t len = strlen(hexStr);
  if (len % 2 != 0) {
    return -1;
  }
  size_t finalLen = len / 2;
  finalLen = (finalLen <= size) ? finalLen : size;
  *outputLen = finalLen;
  for (size_t inIdx = 0, outIdx = 0; outIdx < finalLen; inIdx += 2, outIdx++) {
    if ((hexStr[inIdx] - 48) <= 9 && (hexStr[inIdx + 1] - 48) <= 9) {
      goto convert;
    } else {
      if ((hexStr[inIdx] - 65) <= 5 && (hexStr[inIdx + 1] - 65) <= 5) {
        goto convert;
      } else {
        *outputLen = 0;
        return -1;
      }
    }
  convert:
    buffer[outIdx] =
        (hexStr[inIdx] % 32 + 9) % 25 * 16 + (hexStr[inIdx + 1] % 32 + 9) % 25;
  }
  buffer[finalLen] = '\0';
  return 0;
}
