/*
 * mic_commands.h
 *
 *  Created on: May 15, 2022
 *      Author: DavidWang
 */

#ifndef COMMANDS_MIC_COMMANDS_H_
#define COMMANDS_MIC_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

void mic_read_once_command(int argc, char **argv);
void mic_start_sample_command(int argc, char **argv);
void mic_start_thresh_command(int argc, char **argv);
void mic_stop_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_MIC_COMMANDS_H_ */
