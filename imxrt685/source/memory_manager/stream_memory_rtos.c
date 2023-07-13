/*
 * stream_memory_rtos.c
 *
 *  Created on: Apr 30, 2022
 *      Author: DavidWang
 */

#include <string.h>
#include "stream_memory_rtos.h"
#include "FreeRTOSConfig.h"
#include "loglevels.h"

//static const char *TAG = "SMRTOS"; // Logging prefix for this module

// Indicates bits have been read (it is notifying the writer side)
#define BIT_NOTIFY_WRITE   ( 1 << 0 )
// Indicates bits have been written (notifying the reader side)
#define BIT_NOTIFY_READ   ( 1 << 1 )

void smem_rtos_init (smem_rtos_t* smr, void* buf, size_t buf_cap, size_t max_read_block_size){
  smem_init(&(smr->sm), buf, buf_cap, max_read_block_size);

  smr->sem = xSemaphoreCreateMutexStatic( &(smr->sem_buf) );
  smr->evt_handle = xEventGroupCreateStatic( &(smr->evt_group) );

  smem_rtos_reset(smr);
}

void smem_rtos_reset(smem_rtos_t* smr){

}

/*****************************************************************************/
// From Standard Context

sm_status_t smem_rtos_write_open (smem_rtos_t* smr, size_t req_len, smem_return_buf_t* rdata, TickType_t ticks_to_wait){
//  LOGV(TAG,"smem_rtos_write_open 0");
  sm_status_t status = SM_NO_LOCK;
  while(true){
//    LOGV(TAG,"smem_rtos_write_open 1");
    // take semaphore
    if( xSemaphoreTake( smr->sem, portMAX_DELAY ) == pdTRUE ){
//      LOGV(TAG,"smem_rtos_write_open 2");
      // write open
      status = smem_write_open(&(smr->sm), req_len, rdata);
//      LOGV(TAG,"smem_rtos_write_open 3");
      // give semaphore
      xSemaphoreGive( smr->sem );
//      LOGV(TAG,"smem_rtos_write_open 4");
      if(rdata->ptr != NULL){
//        LOGV(TAG,"smem_rtos_write_open 5");
        // we got a pointer, exit the loop
        break;
      }else{
//        LOGV(TAG,"smem_rtos_write_open 6");
        // we did NOT get a pointer, wait for a notification
        // wait for write notify semaphore
        // TODO: Properly compute the elapsed ticks_to_wait
        EventBits_t uxBits = xEventGroupWaitBits( smr->evt_handle, BIT_NOTIFY_WRITE, pdTRUE, pdFALSE, ticks_to_wait );
//        LOGV(TAG,"smem_rtos_write_open 7");
        if (uxBits & BIT_NOTIFY_WRITE){
          // retry write open
//          LOGV(TAG,"smem_rtos_write_open 8");
        }else{
          // timeout
//          LOGV(TAG,"smem_rtos_write_open 9");
          break;
        }
      }
    }
  }
//  LOGV(TAG,"smem_rtos_write_open %p %u", rdata->ptr, req_len);
  return status;
}

sm_status_t smem_rtos_write(smem_rtos_t* smr, void* actual_start_ptr, size_t actual_len){
//  LOGV(TAG,"smem_rtos_write 0");
  sm_status_t status = SM_NO_LOCK;
  // take semaphore
  if( xSemaphoreTake( smr->sem, portMAX_DELAY ) == pdTRUE ){
    status = smem_write(&(smr->sm), actual_start_ptr, actual_len);
//    LOGV(TAG,"smem_rtos_write %p %u", actual_start_ptr, actual_len);

    // give semaphore
    xSemaphoreGive( smr->sem );
  }
  return status;
}

sm_status_t smem_rtos_write_close(smem_rtos_t* smr){
//  LOGV(TAG,"smem_rtos_write_close 0");
  sm_status_t status = SM_NO_LOCK;
  // take semaphore
  if( xSemaphoreTake( smr->sem, portMAX_DELAY ) == pdTRUE ){
    status = smem_write_close(&(smr->sm));

    // give semaphore
    xSemaphoreGive( smr->sem );
  }
  // give read notify semaphore
  xEventGroupSetBits( smr->evt_handle, BIT_NOTIFY_READ );
  return status;
}

sm_status_t smem_rtos_read_open(smem_rtos_t* smr, size_t req_len, bool req_complete, smem_return_buf_t* rdata, TickType_t ticks_to_wait){
//  LOGV(TAG,"smem_rtos_read_open 0");
  sm_status_t status = SM_NO_LOCK;
  while(true){
//    LOGV(TAG,"smem_rtos_read_open 1");
    // take semaphore
    if( xSemaphoreTake( smr->sem, portMAX_DELAY ) == pdTRUE ){
//      LOGV(TAG,"smem_rtos_read_open 2");
      // read open
      status = smem_read_open(&(smr->sm), req_len, req_complete, rdata);
//      LOGV(TAG,"smem_rtos_read_open 3");
      // give semaphore
      xSemaphoreGive( smr->sem );
//      LOGV(TAG,"smem_rtos_read_open 4");
      if( status==SM_SUCCESS && rdata->size > 0){
//        LOGV(TAG,"smem_rtos_read_open 5");
        // data was read, but other fault
        break;
      }else{
//        LOGV(TAG,"smem_rtos_read_open 6");
        // no data was read
        // wait for read notify semaphore
        // TODO: Properly compute the elapsed ticks_to_wait
        EventBits_t uxBits = xEventGroupWaitBits( smr->evt_handle, BIT_NOTIFY_READ, pdTRUE, pdFALSE, ticks_to_wait );
//        LOGV(TAG,"smem_rtos_read_open 7");
        if (uxBits & BIT_NOTIFY_READ){
//          LOGV(TAG,"smem_rtos_read_open 8");
          // retry open
        }else{
//          LOGV(TAG,"smem_rtos_read_open 9");
          rdata->ptr = NULL;
          rdata->size = 0;
          status = SM_TIMEOUT;
          break;
        }
      }
    }
  }
//  LOGV(TAG,"smem_rtos_read_open %p %u", rdata->ptr, req_len);
  return status;
}

sm_status_t smem_rtos_read_close (smem_rtos_t* smr){
//  LOGV(TAG,"smem_rtos_read_close 0");
  sm_status_t status = SM_NO_LOCK;
  // take semaphore
  if( xSemaphoreTake( smr->sem, portMAX_DELAY ) == pdTRUE ){
    status = smem_read_close(&(smr->sm));

    // give semaphore
    xSemaphoreGive( smr->sem );
  }
  // give write notify semaphore
  xEventGroupSetBits( smr->evt_handle, BIT_NOTIFY_WRITE );
  return status;
}

/*****************************************************************************/
// From ISR Context

