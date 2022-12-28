/*
 * pmic_commands.c
 *
 *  Created on: Dec 27, 2022
 *      Author: DavidWang
 */

#include "pmic_commands.h"
#include "pmic_pca9420.h"

void pmic_status_command(int argc, char **argv){
    pmic_dump_modes();
}



