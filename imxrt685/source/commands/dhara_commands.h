/*
 * dhara_commands.h
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Shell commands for NAND driver.
 *
 */
#ifndef DHARA_COMMANDS_H
#define DHARA_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void dhara_write_read_command(int argc, char **argv);
void dhara_sync_command(int argc, char **argv);
void dhara_clear_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif  // NAND_COMMANDS
