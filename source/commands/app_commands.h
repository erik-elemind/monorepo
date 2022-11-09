/*
 * app_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for app task.
 *
 */
#ifndef APP_COMMANDS_H
#define APP_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void app_event_command(int argc, char **argv);
void app_enter_isp_mode(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif  // APP_COMMANDS
