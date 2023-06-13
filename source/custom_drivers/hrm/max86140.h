/*
 * max86140.h
 *
 *  Created on: Jun 9, 2023
 *      Author: tyler
 */

#ifndef HRM_MAX86140_H_
#define HRM_MAX86140_H_

#include <stdint.h>

#include "loglevels.h"
#include "peripherals.h"
#include "fsl_common.h"
#include "max86140_regs.h"
#include "util_delay.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_FIFO_SAMPLES 128

void max86140_start(void);
void max86140_stop(void);
void max86140_process_fifo(uint8_t* buff, uint8_t* len);
void max86140_test(void);
void max86140_test_read(uint8_t regAdd);
void max86140_test_write(uint8_t regAdd, uint8_t val);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif
#endif /* HRM_MAX86140_H_ */

