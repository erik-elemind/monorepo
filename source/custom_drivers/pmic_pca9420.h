/*
 * pmic_pca9420.h
 *
 *  Created on: Dec 16, 2022
 *      Author: DavidWang
 */

#ifndef PMIC_PCA9420_H_
#define PMIC_PCA9420_H_

#include "fsl_pca9420.h"
#include "fsl_power.h"


#ifdef __cplusplus
extern "C" {
#endif

void pmic_dump_modes(void);
void pmic_init();

#ifdef __cplusplus
}
#endif

#endif /* PMIC_PCA9420_H_ */
