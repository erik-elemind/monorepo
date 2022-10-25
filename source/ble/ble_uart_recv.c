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

#include <ble_uart_recv.h>
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "utils.h"
#include "string_util.h"

#include "FreeRTOS.h"
#include "task.h"

#include "binary_interface_inst.h"
#include "ble_uart_commands.h"  // for extern struct shell_command commands[];

#include "command_parser.h"

#include "loglevels.h"

#if (defined(ENABLE_BLE_UART_RECV_TASK) && (ENABLE_BLE_UART_RECV_TASK > 0U))

//static const char *TAG = "ble_uart_recv"; // Logging prefix for this module

static command_parser_t  ble_uart_recv;


void
ble_uart_handle_input_buf(char* buf, size_t buf_size){
//  LOGV("uart_shell","uart_handle_input_buf");
  ble_uart_recv.index = buf_size;
  size_t min_buf_size = buf_size < BLE_UART_SHELL_CMD_BUFFER_LEN ? buf_size : BLE_UART_SHELL_CMD_BUFFER_LEN;
  memcpy(ble_uart_recv.buf,buf,min_buf_size);
//  LOGV("uart_shell","%s", buf);
  ble_uart_recv.status = PARSER_LINE_FOUND;
  parse_command(&ble_uart_recv);
}


void ble_uart_recv_pretask_init(void){
  vQueueAddToRegistry(USART_BLE_RTOS_HANDLE.rxSemaphore, "ble_uart_rx_sem");
}

/* RTOS task implementation */
void
ble_uart_recv_task(void *ignored)
{
  memset(&ble_uart_recv, 0, sizeof(ble_uart_recv));
  ble_uart_recv.prompt_char = '\0';
  ble_uart_recv.use_prompt = false;

  bin_itf_init();

  while (1) {
    uint8_t byte;
    size_t byte_count;
    int status;

    status = USART_RTOS_Receive(&USART_BLE_RTOS_HANDLE, &byte, sizeof(byte),
      &byte_count);

    if (status != kStatus_Success  || byte_count == 0) {
      vTaskDelay(BLE_UART_SHELL_TASK_DELAY);  // TODO: Try bus reset / recovery?
    } else {

//    LOGV("ble_uart_recv","ble_uart_recv_task: %c %d, status: %d (%d)", (char) byte, (int) byte, status, status==kStatus_Success);

//      char buf[20];
//      snprintf(buf, 20, "R: %d, s:%d", (int)byte, status);
//      debug_uart_puts(buf);

      bin_itf_handle_messages(byte);
    }
  }
}

#else // (defined(ENABLE_BLE_UART_RECV_TASK) && (ENABLE_BLE_UART_RECV_TASK > 0U))

void ble_uart_recv_pretask_init(void){}
void ble_uart_recv_task(void *ignored){}
void ble_uart_handle_input_buf(char* buf, size_t buf_size){}

#endif // (defined(ENABLE_BLE_UART_RECV_TASK) && (ENABLE_BLE_UART_RECV_TASK > 0U))
