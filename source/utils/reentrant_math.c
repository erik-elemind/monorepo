/*
 * reentrant_math.c
 *
 *  Created on: Aug 1, 2021
 *      Author: DavidWang
 */
#include "reentrant_math.h"

#include <math.h> // used for ceil, log10, pow
#include <stdbool.h>

int
rmath_divide_ceil(int num, int den){
  return (num + den - 1) / den;
}

