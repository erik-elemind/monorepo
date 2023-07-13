#ifndef FATFS_COMMANDS_H
#define FATFS_COMMANDS_H

#include <stdbool.h>
#include <stdint.h>

void format_disk_command(int argc, char **argv);
void filetest_command(int argc, char **argv);
void ls_command(int argc, char **argv);
void rm_command(int argc, char **argv);
void printfile(char *filename, int max_bytes);
void cat_command(int argc, char **argv);
void head_command(int argc, char **argv);
void ymodem_command(int argc, char **argv);

#endif  // FATFS_COMMANDS
