/* shell.h - Simple command shell.
 *
 * Copyright (c) 2013, Andrey Yurovsky <yurovsky@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * (ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef __BLE_SHELL_H__
#define __BLE_SHELL_H__

#include "config.h"  // for SHELL_WELCOME_STRING, CONFIG_SHELL_FREERTOS

#ifndef BLE_SHELL_PROMPT_CHR
#define BLE_SHELL_PROMPT_CHR  '#'
#endif

#ifndef BLE_SHELL_TASK_DELAY
#define BLE_SHELL_TASK_DELAY   5
#endif

#define BLE_SHELL_NO_PROMPT

#ifndef SHELL_WELCOME_STRING
  #define SHELL_WELCOME_STRING \
"\n****  Welcome  ****\n\n"\
" Type 'help' or '?' to see a list of shell commands.\n\n"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void ble_shell_pretask_init(void);

#ifdef CONFIG_SHELL_FREERTOS
void ble_shell_task(void *params);
#else
/* Bare-metal implementations must call this ticker function periodically in
 * order to service the shell. */
void ble_shell_task(void);
#endif


void ble_shell_add_char_from_ble(char* buf, size_t buf_size);
int ble_shell_getchar (unsigned long timeoutms);
int ble_shell_putchar (int c);

int ble_shell_putchar_aggregate(int c);
int ble_shell_flush();


#ifdef __cplusplus
}
#endif

#endif /* __SHELL_H__ */
