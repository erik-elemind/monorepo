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

#ifdef __cplusplus
extern "C" {
#endif

void max86140_init(void);
void max86140_test(void);
void max86140_test_read(uint8_t regAdd);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif
#endif /* HRM_MAX86140_H_ */

