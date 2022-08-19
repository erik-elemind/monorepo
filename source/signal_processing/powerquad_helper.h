/*
 * powerquad_helper.h
 *
 *  Created on: Feb 27, 2022
 *      Author: DavidWang
 */

#ifndef SIGNAL_PROCESSING_POWERQUAD_HELPER_H_
#define SIGNAL_PROCESSING_POWERQUAD_HELPER_H_

#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

void pqhelper_arm_rfft_q31(const arm_rfft_instance_q31 *S, q31_t *pSrc, q31_t *pDst);
void pqhelper_init();


#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_PROCESSING_POWERQUAD_HELPER_H_ */
