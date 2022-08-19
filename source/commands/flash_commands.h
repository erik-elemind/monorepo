/*
 * flash_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for NAND SPI flash.
 *
 */
#ifndef FLASH_COMMANDS_H
#define FLASH_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void flash_hello(int argc, char **argv);
void flash_id(int argc, char **argv);
void flash_status(int argc, char **argv);
void flash_unlock(int argc, char **argv);
void flash_read_page(int argc, char **argv);
void flash_write_page(int argc, char **argv);
void flash_check_block(int argc, char **argv);
void flash_check_blocks(int argc, char **argv);
void flash_check_blocks_ecc(int argc, char **argv);
void flash_erase_block(int argc, char **argv);
void flash_erase_blocks(int argc, char **argv);
void flash_mark_bad_block(int argc, char **argv);
void flash_test_complete(int argc, char **argv);
void flash_test_speed(int argc, char **argv);
void flash_test_spi(int argc, char **argv);


#if (defined(ENABLE_NOISE_TEST) && (ENABLE_NOISE_TEST > 0U))
void noise_test_flash_start_command(int argc, char **argv);
void noise_test_flash_stop_command(int argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif

#endif  // FLASH_COMMANDS
