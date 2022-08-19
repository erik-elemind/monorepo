/*
 * ble_fs_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for UFFS.
 *
 */
#ifndef BLE_FS_COMMANDS_H
#define BLE_FS_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void ble_fs_ls_command(int argc, char **argv);
void ble_fs_rm_command(int argc, char **argv);
void ble_fs_mkdir_command(int argc, char *argv[]);
void ble_fs_mv_command(int argc, char *argv[]);
void ble_fs_ymodem_recv_command(int argc, char **argv);
void ble_fs_ymodem_send_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif  // BLE_FS_COMMANDS
