/*
 * stream_memory_rtos.h
 *
 *  Created on: Apr 30, 2022
 *      Author: DavidWang
 */

#ifndef MEMORY_MANAGER_STREAM_MEMORY_RTOS_H_
#define MEMORY_MANAGER_STREAM_MEMORY_RTOS_H_

#include "stream_memory.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  // constant values set by smem_rtos_init()
  smem_t sm;

  // semaphore protecting the write and read functions
  SemaphoreHandle_t sem;
  StaticSemaphore_t sem_buf;

  EventGroupHandle_t evt_handle;
  StaticEventGroup_t evt_group;
}smem_rtos_t;


void smem_rtos_init (smem_rtos_t* smr, void* buf, size_t buf_cap, size_t max_read_block_size);
void smem_rtos_reset(smem_rtos_t* smr);

// Normal Context
sm_status_t smem_rtos_write_open (smem_rtos_t* smr, size_t req_len, smem_return_buf_t* rdata, TickType_t ticks_to_wait);
sm_status_t smem_rtos_write(smem_rtos_t* smr, void* actual_start_ptr, size_t actual_len);
sm_status_t smem_rtos_write_close(smem_rtos_t* smr);

sm_status_t smem_rtos_read_open(smem_rtos_t* smr, size_t req_len, bool req_complete, smem_return_buf_t* rdata, TickType_t ticks_to_wait);
sm_status_t smem_rtos_read_close (smem_rtos_t* smr);


#ifdef __cplusplus
}
#endif

#endif /* MEMORY_MANAGER_STREAM_MEMORY_RTOS_H_ */
