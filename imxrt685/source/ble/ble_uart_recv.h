/* uart_shell.h - Simple command uart_shell.
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
#ifndef __BLE_UART_RECV_H__
#define __BLE_UART_RECV_H__

#include <stddef.h> // for size_t

#ifndef BLE_UART_SHELL_CMD_BUFFER_LEN
#define BLE_UART_SHELL_CMD_BUFFER_LEN  1024
#endif

#ifndef BLE_UART_SHELL_TASK_DELAY
#define BLE_UART_SHELL_TASK_DELAY   5
#endif


#ifdef __cplusplus
extern "C" {
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void ble_uart_recv_pretask_init(void);

void ble_uart_recv_task(void *ignored);

void ble_uart_handle_input_buf(char* buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_UART_RECV_H__ */
