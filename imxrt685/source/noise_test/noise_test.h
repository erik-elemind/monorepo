/*
 * wavbuf.h
 *
 *  Created on: Sep 29, 2020
 *      Author: David Wang
 */

#ifndef _NOISE_TEST__H_
#define _NOISE_TEST__H_

#include "config.h"

#ifndef ENABLE_NOISE_TEST
#define ENABLE_NOISE_TEST 1
#endif

#define NOISE_TEST_FLASH_MAX_NUM_CASES 8


#if (defined(ENABLE_NOISE_TEST) && (ENABLE_NOISE_TEST > 0U))

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif


// Init called before vTaskStartScheduler() launches our Task in main():
void noise_test_pretask_init(void);

void noise_test_task(void *ignored);

void noise_test_flash_start(char* cases);

void noise_test_flash_stop(void);


// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif /* defined(ENABLE_NOISE_TEST) */

#endif /* _NOISE_TEST__H_ */
