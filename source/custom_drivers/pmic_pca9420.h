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
void BOARD_SetPmicVoltageBeforeDeepSleep(void);
void BOARD_RestorePmicVoltageAfterDeepSleep(void);
void BOARD_SetPmicVoltageBeforeDeepPowerDown(void);
/**
 * @brief   Set PMIC volatage for particular frequency.
 * NOTE: The API is only valid when MAINPLLCLKDIV[7:0] and DSPPLLCLKDIV[7:0] are 0.
 *       If LVD falling trip voltage is higher than the required core voltage for particular frequency,
 *       LVD voltage will be decreased to safe level to avoid unexpected LVD reset or interrupt event.
 * @param   tempRange : part temperature range
 * @param   voltOpRange : voltage operation range.
 * @param   cm33Freq : CM33 CPU clock frequency value
 * @param   dspFreq : DSP CPU clock frequency value
 * @return  true for success and false for CPU frequency out of specified voltOpRange.
 */
bool BOARD_SetPmicVoltageForFreq(power_part_temp_range_t tempRange,
                                 power_volt_op_range_t voltOpRange,
                                 uint32_t cm33freq,
                                 uint32_t dspFreq);
#ifdef __cplusplus
}
#endif

#endif /* PMIC_PCA9420_H_ */
