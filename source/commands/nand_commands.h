/*
 * nand_commands.h
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Shell commands for NAND driver.
 *
 */
#ifndef NAND_COMMANDS_H
#define NAND_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void nand_write_read_command(int argc, char **argv);
void nand_safe_wipe_all_blocks_command(int argc, char **argv);
void nand_read_all_blocks_command(int argc, char **argv);
// Raw nand driver printout:
void nand_id_command(int argc, char **argv);
void nand_info_command(int argc, char **argv);
void nand_copy_pages_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif  // NAND_COMMANDS
