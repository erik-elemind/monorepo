/*
 * stream_memory.h
 *
 *  Created on: Apr 23, 2022
 *      Author: DavidWang
 */
#ifndef MEMORY_MANAGER_STREAM_MEMORY_H_
#define MEMORY_MANAGER_STREAM_MEMORY_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
  SM_SUCCESS = 0,
  SM_ERROR,
  SM_TIMEOUT,
  SM_NO_LOCK,
  SM_WRITE_IN_PROGRESS,
  SM_WRITE_NOT_STARTED,
  SM_WRITE_NO_SPACE,
  SM_WRITE_NOTHING,
  SM_WRITE_BW_TAIL_END,
  SM_WRITE_BW_TAIL_HEAD,
  SM_WRITE_TO_ZERO,
  SM_WRITE_OUT_OF_BOUNDS,
  SM_READ_IN_PROGRESS,
  SM_READ_NOT_STARTED,
  SM_READ_NOTHING,
  SM_READ_EMPTY,
  SM_READ_INCOMPLETE,
} sm_status_t;

typedef struct{
  // constant values set by smem_init()
  uint8_t* buf_start;         // pointer to start of buffer
  uint8_t* buf_write_start;   // pointer to start of buffer + read_block_size
  size_t write_capacity;      // the size of the buffer - read_block_size
  size_t max_read_block_size; // the maximum size of readable memory returned by read_open()

  size_t head; // oldest index with data (0 = buf_start+read_block_size)
  size_t tail; // next index in which to store data  (0 = buf_start+read_block_size)
  size_t tail_skipped; // indices equal-to and after this index are unused.
  size_t size;  // number of elements in the stream memory buf

  // write_open(), write_close() cached values
  bool write_open;
  sm_status_t write_strategy;
  uint8_t* block_write_start;
  uint8_t* block_write_end;
  size_t write_total_len;

  // read_open(), read_close() cached values
  bool read_open;
  size_t read_closed_read_size;
  size_t read_closed_new_head;
}smem_t;

typedef struct{
  void* ptr;
  size_t size;
}smem_return_buf_t;


void smem_init (smem_t* sm, void* buf, size_t buf_cap, size_t max_read_block_size);
void smem_reset(smem_t* sm);

sm_status_t smem_write_open (smem_t* sm, size_t req_len, smem_return_buf_t* rdata);
sm_status_t smem_write(smem_t* sm, void* actual_start_ptr, size_t actual_len);
sm_status_t smem_write_close(smem_t* sm);

sm_status_t smem_read_open(smem_t* sm, size_t req_len, bool req_complete, smem_return_buf_t* rdata);
sm_status_t smem_read_close (smem_t* sm);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_MANAGER_STREAM_MEMORY_H_ */
