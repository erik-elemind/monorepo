/*
 * audio_stream_task.cpp
 *
 *  Created on: Mar 9, 2023
 *      Author: DavidWang
 */


#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "loglevels.h"
#include "config.h"
#include "ble.h"
#include "audio_stream_task.h"
#include "AudioPJRC.h"
#include "AudioStream.h"
#include "micro_clock.h"

#if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))

static const char *TAG = "audio_stream";   // Logging prefix for this module

#define AUDIO_STREAM_EVENT_QUEUE_SIZE 50

//
// Task events:
//
typedef enum
{
  // software ISR
  AUDIO_STREAM_EVENT_SOFTWARE_ISR_OCCURRED
} audio_stream_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  audio_stream_event_type_t type;
} audio_stream_event_t;

// Global event queue and handler:
static uint8_t g_event_queue_array[AUDIO_STREAM_EVENT_QUEUE_SIZE*sizeof(audio_stream_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;

void audio_stream_end(void){
    AudioOutputI2S::end();
}

void audio_stream_begin(void){
    AudioOutputI2S::begin();
}

#if 0
#define HALT_IF_DEBUGGING()                              \
  do {                                                   \
    if ((*(volatile uint32_t *)0xE000EDF0) & (1 << 0)) { \
      __asm("bkpt 1");                                   \
    }                                                    \
  } while (0)
#endif

// Non-ISR version of this function
void audio_stream_update(void)
{
  // NOTE: This is called immediately upon boot-up due to I2S DMA ISR, before the scheduler
  // is started. So we guard against that (we can't call xQueueSend() without a scheduler):
  audio_stream_event_t event = {.type = AUDIO_STREAM_EVENT_SOFTWARE_ISR_OCCURRED };
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    xQueueSend(g_event_queue, &event, 0);
  }
}

void audio_stream_update_from_isr(status_t i2s_completion_status)
{
  TRACEALYZER_ISR_AUDIO_BEGIN( AUDIO_I2S_ISR_TRACE );

  if (i2s_completion_status == kStatus_I2S_BufferComplete) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    audio_stream_event_t event = {.type = AUDIO_STREAM_EVENT_SOFTWARE_ISR_OCCURRED };
    xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);

    // Always do this when calling a FreeRTOS "...FromISR()" function:
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }else{
#if 0
    HALT_IF_DEBUGGING();
#endif
  }

  TRACEALYZER_ISR_AUDIO_END( xHigherPriorityTaskWoken );
}


static void
handle_event(audio_stream_event_t *event)
{
  // handle non-state dependent events
  switch (event->type) {
    case AUDIO_STREAM_EVENT_SOFTWARE_ISR_OCCURRED:
        // Handle the software interrupt, but in a scheduled, non-ISR context:
        AudioOutputI2S::handle_audio_i2s_event();

        // TODO: notify the audio task
        audio_event_update_streams();
      return;

    default:
      break;
  }
}

void
audio_stream_pretask_init(void)
{
  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(AUDIO_STREAM_EVENT_QUEUE_SIZE,sizeof(audio_stream_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "audio_stream_event_queue");
}

static void
task_init()
{
  // Any post-scheduler init goes here.

  // Initialize the streaming block
  AudioOutputI2S::init();

  LOGV(TAG, "Task launched. Entering event loop.");
}


void
audio_stream_task(void *ignored)
{

  task_init();

  audio_stream_event_t event;

  while (1) {
    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

//    log_event(&event);

    handle_event(&event);
  }
}


#else // #if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))

void audio_stream_pretask_init(void){}
void audio_stream_task(void *ignored){}

void audio_event_update_streams(void){}
void audio_event_update_streams_from_isr(void){}

#endif // #if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))
