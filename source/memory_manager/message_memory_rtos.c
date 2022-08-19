/*
 * message_memory_rtos.c
 *
 *  Created on: May 4, 2022
 *      Author: DavidWang
 */

/*
 * WARNING: The following code is BUGGY and is NOT ready for general use.
 * Date: May 31st, 2022
 */

#include "message_memory_rtos.h"

//static const char *TAG = "MMRTOS"; // Logging prefix for this module

/*****************************************************************************/
// From Standard Context

/*
void mmem_rtos_init (mmem_rtos_t* mmr, void* buf, size_t buf_cap, size_t max_msg_block_size){
  mmem_init(&(mmr->mm), buf, buf_cap, max_msg_block_size);

  mmr->sem = xSemaphoreCreateMutexStatic( &(mmr->sem_buf) );
  mmr->sem_notify  = xSemaphoreCreateBinaryStatic( &(mmr->sem_notify_buf) );

  mmem_rtos_reset(mmr);
}

void mmem_rtos_reset(mmem_rtos_t* mmr){

}

void* mmem_rtos_write_open (mmem_rtos_t* mmr, size_t requested_len, TickType_t ticks_to_wait){
  void* ptr = NULL;
  while(true){
    // take semaphore
    if( xSemaphoreTake( mmr->sem, portMAX_DELAY ) == pdTRUE ){
      // write open
      ptr=mmem_write_open(&(mmr->mm), requested_len);
      // give semaphore
      xSemaphoreGive( mmr->sem );
      if(ptr!=NULL){
        // we got a pointer, exit the loop
        break;
      }else{
        // we did NOT get a pointer, wait for a notification
        // wait for notify semaphore
        if (xSemaphoreTake( mmr->sem_notify, ticks_to_wait ) != pdTRUE){
          break;
        }
      }
    }
  }
  return ptr;
}

sm_status_t mmem_rtos_write(mmem_rtos_t* mmr, void* actual_start_ptr, size_t actual_len){
  sm_status_t status = SM_SUCCESS;
  // take semaphore
  if( xSemaphoreTake( mmr->sem, portMAX_DELAY ) == pdTRUE ){
    status = mmem_write(&(mmr->mm), actual_start_ptr, actual_len);

    // give semaphore
    xSemaphoreGive( mmr->sem );
  }
  return status;
}

sm_status_t mmem_rtos_write_close(mmem_rtos_t* mmr){
  sm_status_t status = SM_SUCCESS;
  // take semaphore
  if( xSemaphoreTake( mmr->sem, portMAX_DELAY ) == pdTRUE ){
    status = mmem_write_close(&(mmr->mm));

    // give semaphore
    xSemaphoreGive( mmr->sem );
  }
  // give notify semaphore
  xSemaphoreGive( mmr->sem_notify );
  return status;
}

sm_status_t mmem_rtos_read_open(mmem_rtos_t* mmr, smem_return_buf_t* rdata, TickType_t ticks_to_wait){
  sm_status_t status = SM_SUCCESS;
  while(true){
    // take semaphore
    if( xSemaphoreTake( mmr->sem, portMAX_DELAY ) == pdTRUE ){
      // read open
      status = mmem_read_open(&(mmr->mm), rdata);
      // give semaphore
      xSemaphoreGive( mmr->sem );
      if(status==SM_SUCCESS && (rdata->size == 0)){
        // no data was read
        // wait for notify semaphore
        if (xSemaphoreTake( mmr->sem_notify, ticks_to_wait ) != pdTRUE){
          rdata->ptr = NULL;
          rdata->size = 0;
          status = SM_TIMEOUT;
          break;
        }
      }else{
        // data was read, but other fault
        break;
      }
    }
  }
  return status;
}

void mmem_rtos_read_close (mmem_rtos_t* mmr){
  // take semaphore
  if( xSemaphoreTake( mmr->sem, portMAX_DELAY ) == pdTRUE ){
    mmem_read_close(&(mmr->mm));

    // give semaphore
    xSemaphoreGive( mmr->sem );
  }
  // give notify semaphore
  xSemaphoreGive( mmr->sem_notify );
}

*/
