/*
 * als_commands.h
 *
 */

#ifndef ALS_COMMANDS_H_
#define ALS_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

void als_read_once_command(int argc, char **argv);
void als_start_sample_command(int argc, char **argv);
void als_stop_command(int argc, char **argv);
void als_test_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* ALS_COMMANDS_H_ */
