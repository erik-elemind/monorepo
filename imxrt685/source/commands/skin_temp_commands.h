/*
 * skin_temp_commands.h
 *
 *  Created on: May 15, 2022
 *      Author: DavidWang
 */

#ifndef COMMANDS_SKIN_TEMP_COMMANDS_H_
#define COMMANDS_SKIN_TEMP_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

void skin_temp_start_sample_command(int argc, char **argv);
void skin_temp_stop_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_SKIN_TEMP_COMMANDS_H_ */
