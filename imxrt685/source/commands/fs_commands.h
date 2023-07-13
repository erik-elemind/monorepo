/*
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for UFFS.
 *
 */
#ifndef FS_COMMANDS_H
#define FS_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void fs_write_command(int argc, char **argv);
void fs_read_command(int argc, char **argv);
void fs_format_command(int argc, char **argv);
void fs_touch_command(int argc, char **argv);
void fs_ls_command(int argc, char **argv);
void fs_rm_command(int argc, char **argv);
void fs_rm_datalogs_command(int argc, char **argv);
void fs_rm_all_command(int argc, char *argv[]);
void fs_mv_command(int argc, char **argv);
void fs_mkdir_command(int argc, char *argv[]);
void fs_cp_command(int argc, char **argv);
void fs_cat_command(int argc, char **argv);
void fs_info_command(int argc, char **argv);
void fs_ymodem_recv_command(int argc, char **argv);
void fs_ymodem_send_command(int argc, char **argv);
void fs_ymodem_recv_test_command(int argc, char **argv);
void fs_ymodem_send_test_command(int argc, char **argv);

void fs_zmodem_send_command(int argc, char *argv[]);
void fs_zmodem_recv_command(int argc, char *argv[]);
void fs_zmodem_recv_test_command(int argc, char **argv);
void fs_zmodem_send_test_command(int argc, char **argv);
#if (defined(ENABLE_FS_TEST_COMMANDS) && (ENABLE_FS_TEST_COMMANDS > 0U))
void fs_test_speed_command(int argc, char **argv);
void fs_test_mixedrw_speed_command(int argc, char **argv);
void fs_test_filling_write_read_command(int argc, char **argv);
//void fs_test_nand_write_read_command(int argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif

#endif  // FS_COMMANDS
