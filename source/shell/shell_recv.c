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

#include <shell_recv.h>
#include <stdio.h>
#include <string.h>
#include "virtual_com.h"

#include "app.h"
#include "utils.h"
#include "command_parser.h"

#ifdef CONFIG_SHELL_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

static command_parser_t shell;

void shell_recv_pretask_init(void)
{
#if defined(CONFIG_SHELL_USB)
  virtual_com_init();
#endif
}

#ifdef CONFIG_SHELL_FREERTOS
/* RTOS task implementation */
void shell_recv_task(void *params)
{
  memset(&shell, 0, sizeof(shell));
  shell.prompt_char = SHELL_PROMPT_CHR;

  /* USB-RTC Contention Issue.
   *
   * There are currently 3 possible solutions:
   * Solution 1: Use a 12 second delay after the device powers on before the USB connection starts, delay occurs BEFORE scheduler start.
   *             Implemented in virtual_com.c, end of virtual_com_init_helper() function.
   * Solution 2: Use a 12 second delay after the device powers on before the USB connection starts, delay occurs AFTER scheduler start.
   *             Implemented in shell_recv.c, at the start of shell_recv_task() function.
   * Solution 3: In clock_config.c (variants/variant_ff2/clock_config.c), in function BOARD_BootClockPLL150M(),
   *             comment-out the line 'CLOCK_AttachClk(kXTAL32K_to_OSC32K);' - which is a partial solution that fixed USB but disables RTC
   *             unless additional code is added to switch to the 32kHZ FRO.
   *
   * The delay time of 12 seconds was arrived at experimentally.
   * <= 10000 ms =  causes USB 3.0 init error on David's Windows 10 computer
   * >= 11000 ms = WORKS
   * using 12000 for safety.
   *             */


  // Give the initial debug logs some time to finish printing...
   vTaskDelay((300)/portTICK_PERIOD_MS);

#ifdef SHELL_WELCOME_STRING
  puts(SHELL_WELCOME_STRING);
#endif
  prompt(&shell);

  while (1) {
    int c;

    c = getchar();
    if (c == EOF) {
       vTaskDelay(SHELL_TASK_DELAY);
    } else {
      // The user pressed a key, so stay awake
      //deepsleep_timer_reset();
      handle_input(&shell,(char)c);
      fflush(stdout);
    }
  }
}
#else
/* bare-metal "tick" function impelmentation */
void shell_recv_task(void)
{
  int c;
  static int shell_run = 0;

  /* one-time prompt */
  if (!shell_run) {
#ifdef SHELL_WELCOME_STRING
    puts(SHELL_WELCOME_STRING);
#endif
    prompt();

    shell_run = 1;
  }

  /* shell "tick" function */
  c = getchar();
  if (c != EOF) {
    handle_input((char)c);
  }
}
#endif
