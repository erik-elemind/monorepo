/*
 * filehash_commands.h
 *
 *  Created on: Apr 17, 2022
 *      Author: DavidWang
 */

#ifndef COMMANDS_FILEHASH_COMMANDS_H_
#define COMMANDS_FILEHASH_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

void filehash_sha256_command(int argc, char **argv);
void ble_filehash_sha256_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_FILEHASH_COMMANDS_H_ */
