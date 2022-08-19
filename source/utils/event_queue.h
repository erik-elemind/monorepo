/*
 * data_queue.h
 *
 *  Created on: Aug 25, 2021
 *      Author: DavidWang
 */

#ifndef UTILS_EVENT_QUEUE_H_
#define UTILS_EVENT_QUEUE_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  // event queue
  StaticQueue_t event_struct;
  QueueHandle_t event_handle;
  // storage
  uint8_t* data_storage;
  size_t data_len;
  size_t data_size;
  size_t head;
  size_t tail;
  // semaphores
  SemaphoreHandle_t sem;
  StaticSemaphore_t sem_buffer;
} event_queue_t;

typedef struct
{
  void* item;
  void* data;
  size_t data_len;
} event_t;





#ifdef __cplusplus
}
#endif

#endif /* UTILS_EVENT_QUEUE_H_ */
