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

#include <stdio.h>
#include <string.h>
#include <virtual_com_OLD.h>
#include "ble_shell.h"

#include "app.h"
#include "utils.h"
#include "command_parser.h"

#ifdef CONFIG_SHELL_FREERTOS
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"
#endif

#include "binary_interface_inst.h"
#include "../commands/fs_commands.h"

#define BLE_RECEIVE_STORAGE_SIZE (2048) // should be larger than 1024+5 bytes for Ymodem send

static uint8_t ucBufferStorage[ BLE_RECEIVE_STORAGE_SIZE ];
static StaticStreamBuffer_t xStreamBufferStruct;
static StreamBufferHandle_t xStreamBuffer;

static command_parser_t ble_shell;

void ble_shell_pretask_init(void)
{
  const size_t xTriggerLevel = 1;
  xStreamBuffer = xStreamBufferCreateStatic( sizeof(ucBufferStorage),
                                                       xTriggerLevel,
                                                       ucBufferStorage,
                                                       &(xStreamBufferStruct) );
}

#ifdef CONFIG_SHELL_FREERTOS
/* RTOS task implementation */
void ble_shell_task(void *params)
{
  char *sstr = "fs_zmodem_send_test";
  char *rstr = "fs_zmodem_recv_test";
  memset(&ble_shell, 0, sizeof(ble_shell));
  ble_shell.prompt_char = '\0';
  ble_shell.use_prompt = false;

  // Give the initial debug logs some time to finish printing...
  vTaskDelay((300)/portTICK_PERIOD_MS);

#ifdef SHELL_WELCOME_STRING
  puts(SHELL_WELCOME_STRING);
#endif
  prompt(&ble_shell);

  while (1) {
    int rc;

    rc = ble_shell_getchar(0);

    // The following code was added to test zmodem, but interferes with reliable ble communication
    // Specifically, it causes the command "ble_filehash_sha256" to fail.
#if 0
    // TODO: Modify the Zmodem protocol or introduce new test protocol 
    // so this hack is not required to test zmodem.
    if(rc=='s')
    {
      fs_zmodem_send_test_command(1, &sstr);
    }
    else if(rc=='r')
    {
      fs_zmodem_recv_test_command(1, &rstr);
    }
#endif


    if (rc == EOF) {
      vTaskDelay(BLE_SHELL_TASK_DELAY);
    } else {
      // The user pressed a key, so stay awake
      //deepsleep_timer_reset();
      handle_input(&ble_shell, (char)rc);
//      fflush(stdout);
    }

  }
}
#else
/* bare-metal "tick" function impelmentation */
void ble_shell_task(void)
{
  int c;
  static int shell_run = 0;

  /* one-time prompt */
  if (!shell_run) {
#ifdef SHELL_WELCOME_STRING
    puts(SHELL_WELCOME_STRING);
#endif
    prompt(&ble_shell);

    shell_run = 1;
  }

  /* shell "tick" function */
  c = getchar();
  if (c != EOF) {
    handle_input(&ble_shell, (char)c);
  }
}
#endif


void
ble_shell_add_char_from_ble(char* buf, size_t buf_size)
{
  xStreamBufferSend(xStreamBuffer, buf, buf_size, portMAX_DELAY);
}


int ble_shell_getchar (unsigned long timeoutms)
{
  char ch;
  size_t xReceivedBytes = xStreamBufferReceive( xStreamBuffer, &ch, 1, pdMS_TO_TICKS(timeoutms)); //portMAX_DELAY
    if(xReceivedBytes != 1){
    return -1;
    }else{
    return ch;
  }
}


int ble_shell_putchar (int c)
{
  // push over bluetooth
  char ch = (char) c;
    if( bin_itf_send_uart_command(&ch, (size_t)1) ) {
    return 1;
    } else {
    return -1;
  }
}

#define AGG_BUFFER_SIZE 230 // this should probably be less than 254 the upper limit for our COBS implementation
char agg_buffer[AGG_BUFFER_SIZE+1];
size_t agg_buffer_i = 0;

int ble_shell_putchar_aggregate(int c){
  bool success = false;
  if(agg_buffer_i >= AGG_BUFFER_SIZE){
    success = ble_shell_flush();
  }

  agg_buffer[agg_buffer_i] = c;
  agg_buffer[agg_buffer_i+1] ='\0';
  agg_buffer_i++;
  success = true;

  return success ? 1 : -1;
}

int ble_shell_flush(){
  bool success = false;
  if(agg_buffer_i > 0){
    success = bin_itf_send_uart_command(agg_buffer, agg_buffer_i);
    agg_buffer_i = 0;
  }else{
    success = true;
  }
  return success ? 1 : -1;
}

