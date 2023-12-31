/*
 * pmic_commands.h
 *
 *  Created on: Dec 27, 2022
 *      Author: DavidWang
 */

#ifndef PMIC_COMMANDS_H_
#define PMIC_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

void pmic_status_command(int argc, char **argv);
void pmic_test_command(int argc, char **argv);
void pmic_enter_ship_mode_command(int argc, char **argv);
void pmic_batt_status(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* PMIC_COMMANDS_H_ */
