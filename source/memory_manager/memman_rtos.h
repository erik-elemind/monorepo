/*
 * memman_rtos.h
 *
 *  Created on: May 16, 2022
 *      Author: DavidWang
 */

#ifndef MEMORY_MANAGER_MEMMAN_RTOS_H_
#define MEMORY_MANAGER_MEMMAN_RTOS_H_

#include "memman.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "portmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  mm_t mm;

  SemaphoreHandle_t sem;
  StaticSemaphore_t sem_buf;

  EventGroupHandle_t evt_handle;
  StaticEventGroup_t evt_group;
} mm_rtos_t;

int mm_rtos_init (mm_rtos_t* mm, void* buf, size_t buf_size);
void *mm_rtos_malloc (mm_rtos_t* mm, size_t size, TickType_t ticks_to_wait);
void mm_rtos_free (mm_rtos_t* mm, void *ptr);
void mm_rtos_exit (mm_rtos_t* mm);

// ISR context
void *mm_rtos_malloc_from_isr (mm_rtos_t* mm, size_t size, BaseType_t *pxHigherPriorityTaskWoken);
void mm_rtos_free_from_isr (mm_rtos_t* mm, void *ptr, BaseType_t *pxHigherPriorityTaskWoken);

#ifdef __cplusplus
}
#endif




#endif /* MEMORY_MANAGER_MEMMAN_RTOS_H_ */
