/*
 * nand_commands.h
 *
 * Copyright (C) 2020 Igor Institute, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger, Derek Simkowiak
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
void nand_hello_command(int argc, char **argv);
void nand_info_command(int argc, char **argv);
void nand_copy_pages_command(int argc, char **argv);

void nand_unlock_command(int argc, char **argv);
void nand_mux_select_command(int argc, char **argv);
void nand_read_page_command(int argc, char **argv);
void nand_write_page_command(int argc, char **argv);
void nand_check_block_command(int argc, char **argv);
void nand_check_blocks_command(int argc, char **argv);
void nand_check_blocks_ecc_command(int argc, char **argv);
void nand_erase_block_command(int argc, char **argv);
void nand_erase_blocks_command(int argc, char **argv);
void nand_mark_bad_block_command(int argc, char **argv);
void nand_test_complete_command(int argc, char **argv);
void nand_test_speed_command(int argc, char **argv);
void nand_test_spi_command(int argc, char **argv);


#if (defined(ENABLE_NOISE_TEST) && (ENABLE_NOISE_TEST > 0U))
void noise_test_flash_start_command(int argc, char **argv);
void noise_test_flash_stop_command(int argc, char **argv);
#endif


#ifdef __cplusplus
}
#endif

#endif  // NAND_COMMANDS
