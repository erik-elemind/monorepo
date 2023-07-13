/*
 * memman_rtos.c
 *
 *  Created on: May 16, 2022
 *      Author: DavidWang
 */

#include <stdbool.h>
#include "memman_rtos.h"

#define BIT_NOTIFY_MALLOC   ( 1 << 0 )

int mm_rtos_init (mm_rtos_t* mm, void* buf, size_t buf_size){
  int status = mm_init(&(mm->mm), buf, buf_size);
  mm->sem = xSemaphoreCreateMutexStatic( &(mm->sem_buf) );
  mm->evt_handle = xEventGroupCreateStatic( &(mm->evt_group) );
  return status;
}

void *mm_rtos_malloc (mm_rtos_t* mm, size_t size, TickType_t ticks_to_wait){
  void* ptr = NULL;
  while(true){
    // take semaphore
    if( xSemaphoreTake( mm->sem, portMAX_DELAY) == pdTRUE ){ // the mem_brk address, i.e the start of the header, contains a top 4 bytes and a bottom 4 bytes, where the last 3 bits (since they are always aligned on boundaries) are flags
      // allocate memory
      ptr = mm_malloc(&(mm->mm), size);
      // give semaphore
      xSemaphoreGive( mm->sem ); 
      if(ptr != NULL){
        // we got a pointer, exit the loop
        break;
      }else{
        // we did NOT get a pointer, wait for a notification
        // wait for memory available notify semaphore
        // TODO: Properly compute the elapsed ticks_to_wait
        EventBits_t uxBits = xEventGroupWaitBits( mm->evt_handle, BIT_NOTIFY_MALLOC, pdTRUE, pdFALSE, ticks_to_wait );
        if (uxBits & BIT_NOTIFY_MALLOC){
          // retry malloc
        }else{
          // timeout
          break;
        }
      }
    }
  }
  return ptr;
}

void mm_rtos_free (mm_rtos_t* mm, void *ptr){
  if( xSemaphoreTake( mm->sem, portMAX_DELAY) == pdTRUE ){
    // free memory
    mm_free(&(mm->mm), ptr);
    // give semaphore
    xSemaphoreGive( mm->sem );
    // notify memory available
    xEventGroupSetBits( mm->evt_handle, BIT_NOTIFY_MALLOC );
  }
}

void mm_rtos_exit (mm_rtos_t* mm){
  mm_exit( &(mm->mm) );
}


// ISR context
void *mm_rtos_malloc_from_isr (mm_rtos_t* mm, size_t size, BaseType_t *pxHigherPriorityTaskWoken){
  BaseType_t higherTaskTake = pdFALSE;
  BaseType_t higherTaskGive = pdFALSE;

  void* ptr = NULL;

  // take semaphore
  if( xSemaphoreTakeFromISR( mm->sem, &higherTaskTake) == pdTRUE ){
    // allocate memory
    ptr = mm_malloc(&(mm->mm), size);
    // give semaphore
    xSemaphoreGiveFromISR( mm->sem , &higherTaskGive );
  }

  return ptr;

  *pxHigherPriorityTaskWoken = higherTaskTake || higherTaskGive;
}

void mm_rtos_free_from_isr (mm_rtos_t* mm, void *ptr, BaseType_t *pxHigherPriorityTaskWoken){
  BaseType_t higherTaskTake = pdFALSE;
  BaseType_t higherTaskGive = pdFALSE;
  BaseType_t higherTaskGroup = pdFALSE;

  if( xSemaphoreTakeFromISR( mm->sem, &higherTaskTake) == pdTRUE ){
    // free memory
    mm_free(&(mm->mm), ptr);
    // give semaphore
    xSemaphoreGiveFromISR( mm->sem , &higherTaskGive);
    // notify memory available
    xEventGroupSetBitsFromISR( mm->evt_handle, BIT_NOTIFY_MALLOC, &higherTaskGroup );
  }

  *pxHigherPriorityTaskWoken = higherTaskTake || higherTaskGive || higherTaskGroup;
}
