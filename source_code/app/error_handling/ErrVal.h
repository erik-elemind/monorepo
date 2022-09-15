/*
 * ErrVal.h
 *
 *  Created on: Dec 28, 2018
 *      Author: david
 */

#ifndef SRC_ERRVAL_H_
#define SRC_ERRVAL_H_

#include "sdk_common.h"
#include "ErrCodes.h"

typedef struct ErrValUINT {
	error_t error_;
	unsigned int value_;
} ErrValUINT;

typedef struct ErrValUINT8 {
	error_t error_;
	uint8_t value_;
} ErrValUINT8;

typedef struct ErrValUINT16 {
	error_t error_;
	uint16_t value_;
} ErrValUINT16;

typedef struct ErrValUINT32 {
	error_t error_;
	uint32_t value_;
} ErrValUINT32;

typedef struct ErrValUINT64 {
	error_t error_;
	uint64_t value_;
} ErrValUINT64;

typedef struct ErrValINT {
	error_t error_;
	int value_;
} ErrValINT;

typedef struct ErrValINT8 {
	error_t error_;
	int8_t value_;
} ErrValINT8;

typedef struct ErrValINT16 {
	error_t error_;
	int16_t value_;
} ErrValINT16;

typedef struct ErrValINT32 {
	error_t error_;
	int32_t value_;
} ErrValINT32;

typedef struct ErrValINT64 {
	error_t error_;
	int64_t value_;
} ErrValINT64;

typedef struct ErrValFLOAT {
	error_t error_;
	float value_;
} ErrValFLOAT;

typedef struct ErrValDOUBLE {
	error_t error_;
	double value_;
} ErrValDOUBLE;

typedef struct ErrValBOOL {
	error_t error_;
	bool value_;
} ErrValBOOL;



#endif /* SRC_ERRVAL_H_ */
