/*
 * compression_algo.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Feb, 2022
 * Author:  Dinesh Ribadiya
 */

#ifndef COMPRESSION_ALGO_H
#define COMPRESSION_ALGO_H

#include <stddef.h>
#include "compression_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void compression_test_task_test1(void);
void compression_test_task_test2(void);
void compression_test_task_test3(void);

// Init called before vTaskStartScheduler() launches our Task in main():
void compression_test_pretask_init(void);

void compression_test_task(void *ignored);

#ifdef __cplusplus
}
#endif


#endif  // COMPRESSION_ALGO_H
