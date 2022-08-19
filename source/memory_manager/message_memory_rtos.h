/*
 * message_memory_rtos.h
 *
 *  Created on: May 4, 2022
 *      Author: DavidWang
 */

/*
 * WARNING: The following code is BUGGY and is NOT ready for general use.
 * Date: May 31st, 2022
 */

#ifndef MEMORY_MANAGER_MESSAGE_MEMORY_RTOS_H_
#define MEMORY_MANAGER_MESSAGE_MEMORY_RTOS_H_

#include "message_memory.h"
#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*

typedef struct{
  // constant values set by mmem_init()
  mmem_t mm;

  // semaphore protecting the write and read functions
  SemaphoreHandle_t sem;
  StaticSemaphore_t sem_buf;

  SemaphoreHandle_t sem_notify;
  StaticSemaphore_t sem_notify_buf;
}mmem_rtos_t;

void mmem_rtos_init (mmem_rtos_t* mmr, void* buf, size_t buf_cap, size_t max_msg_block_size);
void mmem_rtos_reset(mmem_rtos_t* mmr);

// Normal Context
void* mmem_rtos_write_open (mmem_rtos_t* mmr, size_t requested_len, TickType_t ticks_to_wait);
sm_status_t mmem_rtos_write(mmem_rtos_t* mmr, void* actual_start_ptr, size_t actual_len);
sm_status_t mmem_rtos_write_close(mmem_rtos_t* mmr);

sm_status_t mmem_rtos_read_open(mmem_rtos_t* mmr, smem_return_buf_t* rdata, TickType_t ticks_to_wait);
void mmem_rtos_read_close (mmem_rtos_t* mmr);

// From ISR Context
void* mmem_rtos_write_open_fromisr (mmem_rtos_t* mmr, size_t requested_len, TickType_t ticks_to_wait);
sm_status_t mmem_rtos_write_fromisr(mmem_rtos_t* mmr, void* actual_start_ptr, size_t actual_len);
sm_status_t mmem_rtos_write_close_fromisr(mmem_rtos_t* mmr);

sm_status_t mmem_rtos_read_open_fromisr(mmem_rtos_t* mmr, smem_return_buf_t* rdata, TickType_t ticks_to_wait);
void mmem_rtos_read_close_fromisr (mmem_rtos_t* mmr);

*/

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_MANAGER_MESSAGE_MEMORY_RTOS_H_ */
