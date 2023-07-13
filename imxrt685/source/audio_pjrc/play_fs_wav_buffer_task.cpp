/*
 * wavbuf.cpp
 *
 *  Created on: Sep 29, 2020
 *      Author: David Wang
 */


#include <play_fs_wav_buffer_rtos.h>
#include <play_fs_wav_buffer_task.h>
#include <play_fs_wav_buffer_task.h>
#include "loglevels.h"
#include "utils.h"

#if (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK))

TaskHandle_t g_wavbuf_task_handle = NULL;

static const char *TAG = "wavbuf";   // Logging prefix for this module


void
wavbuf_pretask_init(void)
{
  // Any pre-scheduler init goes here.
  AudioPlayFsWavBufferRTOS::pretask_init_all_buffers();
}


static void
task_init()
{
  // Any post-scheduler init goes here.
  g_wavbuf_task_handle = xTaskGetCurrentTaskHandle();

  LOGV(TAG, "Task launched. Entering event loop.");
}

void wavbuf_task_wakeup()
{
  // Wake up the sleeping task
  xTaskNotify( g_wavbuf_task_handle,
               0x0,  // ignored
               eNoAction );
}


void
wavbuf_task(void *ignored)
{
  task_init();

  uint32_t notification_value;

  while (1) {

    // Stop when fill_all_buffers returns false
    // which indicates no buffers are "active".
    bool active = AudioPlayFsWavBufferRTOS::fill_all_buffers();
    if (!active){
      // Wait for a start notification
      xTaskNotifyWait( 0,                    // Clear no bits on entry
                       UINT32_MAX,            // Clear all bits on exit
                       &notification_value,  // ignored
                       portMAX_DELAY );
    }

  }
}

#endif // (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK))
