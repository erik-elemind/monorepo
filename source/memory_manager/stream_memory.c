/*
 * stream_memory.c
 *
 *  Created on: Apr 23, 2022
 *      Author: DavidWang
 */

#include <string.h>
#include "stream_memory.h"
#include "FreeRTOSConfig.h"
#include "loglevels.h"

//static const char *TAG = "SM"; // Logging prefix for this module

static size_t min(size_t a, size_t b) {
  return a < b ? a : b;
}

void smem_init (smem_t* sm, void* buf, size_t buf_cap, size_t max_read_block_size){
  configASSERT( (buf != NULL) && (buf_cap != 0));
  configASSERT( buf_cap > max_read_block_size );
  sm->buf_start           = ((uint8_t*)buf);
  sm->buf_write_start     = ((uint8_t*)buf) + max_read_block_size;
  sm->write_capacity      = buf_cap - max_read_block_size;
  sm->max_read_block_size = max_read_block_size;

  smem_reset(sm);
}

void smem_reset(smem_t* sm){
  sm->head              = 0;
  sm->tail              = 0;
  sm->tail_skipped      = sm->write_capacity;
  sm->size              = 0;
  sm->write_open        = false;
  sm->read_open         = false;
}

static sm_status_t sm_compute_write_strategy(smem_t* sm, size_t len){
  // If len is 0, do nothing
  if (len <= 0){
    return SM_WRITE_NOTHING;
  }

  if ( (sm->head < sm->tail) || (sm->head==sm->tail && sm->size==0) ){
    // head is before tail or buffer is empty
    if ( sm->tail + len <= sm->write_capacity ){
      return SM_WRITE_BW_TAIL_END;
    }else if( len <= sm->head ){
      return SM_WRITE_TO_ZERO;
    }else{
      return SM_WRITE_NO_SPACE;
    }
  }else if (sm->head > sm->tail){
    // head is after tail (data spans end of buffer)
    if ( sm->tail + len <= sm->head ){
      return SM_WRITE_BW_TAIL_HEAD;
    }else{
      return SM_WRITE_NO_SPACE;
    }
  }
  // This happens if head == tail.
  return SM_WRITE_NO_SPACE;
}

/*
 *
 * Returns:
 * SM_WRITE_IN_PROGRESS - could not write, write stays closed
 * SM_WRITE_NO_SPACE - no space available for write, write stays closed
 * SM_WRITE_NOTHING - the requested len is 0, write stays closed
 * SM_SUCCESS - write open
 * SM_ERROR - this should never happen
 */
sm_status_t smem_write_open (smem_t* sm, size_t req_len, smem_return_buf_t* rdata){
  sm_status_t status = SM_ERROR;
  rdata->ptr = NULL;
  rdata->size = 0;
  void* ptr = NULL;
  // take semaphore
  if((sm->write_open)){
    status = SM_WRITE_IN_PROGRESS;
  }else{
    sm->write_strategy = sm_compute_write_strategy(sm, req_len);

    sm->write_total_len = 0;
    sm->block_write_start = NULL;
    sm->block_write_end = NULL;
    switch(sm->write_strategy){
    case SM_WRITE_NO_SPACE:
      status = SM_WRITE_NO_SPACE;
      break;
    case SM_WRITE_NOTHING:
      status = SM_WRITE_NOTHING;
      break;
    case SM_WRITE_BW_TAIL_END:
    case SM_WRITE_BW_TAIL_HEAD:
      // data fits between tail and end of buffer
      ptr = &(sm->buf_write_start[sm->tail]);
      // save requested write length
      sm->block_write_start = ptr;
      sm->block_write_end = sm->block_write_start + req_len;
      // write is open
      sm->write_open = true;
      // save return data
      rdata->ptr = ptr;
      rdata->size = req_len;
      status = SM_SUCCESS;
      break;
    case SM_WRITE_TO_ZERO:
      // data fits between start of buffer and head
      ptr = &(sm->buf_write_start[0]);
      // save requested write length
      sm->block_write_start = ptr;
      sm->block_write_end = sm->block_write_start + req_len;
      // write is open
      sm->write_open = true;
      // save return data
      rdata->ptr = ptr;
      rdata->size = req_len;
      status = SM_SUCCESS;
      break;
    default:
      // TODO: return an error indicating the strategy was unrecognized
      status = SM_ERROR;
      break;
    }
  }
  return status;
}

/*
 *
 * Returns:
 * SM_WRITE_NOT_STARTED - write not open, nothing written
 * SM_SUCCESS - data written
 * SM_WRITE_OUT_OF_BOUNDS - the ptr and actual_len are out of bounds, nothing written
 * SM_ERROR - this should never happen
 */
sm_status_t smem_write(smem_t* sm, void* actual_start_ptr, size_t actual_len){
  sm_status_t status = SM_ERROR;
  // take semaphore
  if(!(sm->write_open)){
    status = SM_WRITE_NOT_STARTED;
  }else{
    // consolidate the data
    uint8_t* actual_write_start = (uint8_t*) actual_start_ptr;
    if( sm->block_write_start == actual_write_start &&
        (actual_write_start+actual_len <= sm->block_write_end)){
      // no copy
      // update write cache values
      sm->block_write_start += actual_len;
      sm->write_total_len += actual_len;
      status = SM_SUCCESS;
    }else if( (sm->block_write_start < actual_write_start) &&
        (actual_write_start+actual_len <= sm->block_write_end)){
      // copy data to front of write block
      memmove(sm->block_write_start, actual_write_start, actual_len);
      // update write cache values
      sm->block_write_start += actual_len;
      sm->write_total_len += actual_len;
      status = SM_SUCCESS;
    }else{
      status = SM_WRITE_OUT_OF_BOUNDS;
    }
  }
  return status;
}

/*
 *
 * SM_WRITE_NOT_STARTED - write not open, nothing written
 * SM_WRITE_NO_SPACE - no space available for write, nothing written
 * SM_WRITE_NOTHING - the requested len is 0, nothing written
 * SM_SUCCESS - data written successfully
 * SM_ERROR - this will only happen if the write strategy is unrecognized
 */
sm_status_t smem_write_close(smem_t* sm){
  if(sm->head < sm->tail){
    configASSERT(sm->tail - sm->head == sm->size );
  }else if(sm->head > sm->tail){
    configASSERT( ((sm->tail_skipped - sm->head) + sm->tail) == sm->size );
  }

  sm_status_t status = SM_ERROR;
  // take semaphore
  if(!(sm->write_open)){
    status = SM_WRITE_NOT_STARTED;
  }else{
    // write open flag must be set false on a call to close
    sm->write_open = false;
    // update the tail pointer.
    size_t actual_len_from_zero = sm->write_total_len;
    // act based on recorded write strategy
    switch(sm->write_strategy){
    case SM_WRITE_NO_SPACE:
      status = SM_WRITE_NO_SPACE;
      break;
    case SM_WRITE_NOTHING:
      status = SM_WRITE_NOTHING;
      break;
    case SM_WRITE_BW_TAIL_END:
      // data fits between tail and end of buffer
      // update the tail pointer.
      sm->tail += actual_len_from_zero;
      // update size
      sm->size += actual_len_from_zero;
      // update status
      status = SM_SUCCESS;
      break;
    case SM_WRITE_BW_TAIL_HEAD:
      // data fits between tail and end of buffer
      // update the tail pointer.
      sm->tail += actual_len_from_zero;
      // update size
      sm->size += actual_len_from_zero;
      // update status
      status = SM_SUCCESS;
      break;
    case SM_WRITE_TO_ZERO:
      // data fits between start of buffer and head
      // record the tail pointer that we are skipping
      sm->tail_skipped = sm->tail;
      // update the tail pointer.
      sm->tail = actual_len_from_zero;
      // update size
      sm->size += actual_len_from_zero;
      // update status
      status = SM_SUCCESS;
      break;
    default:
      // do nothing
      // TODO: return an error indicating the strategy was unrecognized
      status = SM_ERROR;
      break;
    }
  }
  return status;
}

/*
 *
 * SM_READ_IN_PROGRESS
 * SM_READ_NOTHING
 * SM_READ_EMPTY
 * SM_SUCCESS - read is open, rdata has a good value
 */
sm_status_t smem_read_open(smem_t* sm, size_t req_len, bool req_complete, smem_return_buf_t* rdata){
  configASSERT(req_len <= sm->max_read_block_size);
//  LOGV("stream_memory", "smem_read_open, req_len: %u, head: %u, tail: %u, tail_skip: %u, size: %u, write_cap: %u", req_len, sm->head, sm->tail, sm->tail_skipped, sm->size, sm->write_capacity);
  if(sm->head < sm->tail){
    configASSERT(sm->tail - sm->head == sm->size );
  }else if(sm->head > sm->tail){
    configASSERT( ((sm->tail_skipped - sm->head) + sm->tail) == sm->size );
  }

  sm_status_t status = SM_SUCCESS;
  // take semaphore
  if((sm->read_open)){
    status = SM_READ_IN_PROGRESS;
  }else{
    sm->read_closed_read_size = 0;
    sm->read_closed_new_head = sm->head;
    rdata->ptr = NULL;
    if (req_len <= 0){
      // If len is 0, do nothing
      // SM_REMOVE_NOTHING
      status = SM_READ_NOTHING;
    }else if(sm->head == sm->tail && sm->size==0){
      // SM_REMOVE_EMPTY
      status = SM_READ_EMPTY;
    }else if(req_len > sm->size && req_complete){
      // SM_READ_INCOMPLETE
      status = SM_READ_INCOMPLETE;
    }else if (sm->head < sm->tail){
      // head is before tail
      rdata->ptr = &(sm->buf_write_start[sm->head]);
      sm->read_closed_read_size = min(sm->tail - sm->head, req_len);
      sm->read_closed_new_head = sm->head + sm->read_closed_read_size;
    }else{
      // head is after tail (data spans end of buffer) or buffer is full
      // compare head to tail_skipped
      size_t head_after_req = sm->head + req_len;
      if ( head_after_req == sm->tail_skipped ){
        // there is enough contiguous data left.
        sm->read_closed_read_size = req_len;
        sm->read_closed_new_head = 0;
        rdata->ptr = &(sm->buf_write_start[sm->head]);
      }else if ( head_after_req < sm->tail_skipped ){
        // there is enough contiguous data left.
        sm->read_closed_read_size = req_len;
        sm->read_closed_new_head = sm->head + req_len;
        rdata->ptr = &(sm->buf_write_start[sm->head]);
      }else if ( head_after_req > sm->tail_skipped ){
        // There is NOT enough contiguous data left in the tail
        // Shift the memory to the front of the buffer to create a contiguous array to return
        size_t remaining_elements = sm->tail_skipped - sm->head;
        if(remaining_elements != 0){
          memmove(sm->buf_write_start-remaining_elements, &(sm->buf_write_start[sm->head]), remaining_elements);
        }
        // It would be more efficient for the following code to be written as:
        // sm->read_closed_read_size = min(req_len, sm->size);
        // but it is written as follows in case we only want to add an argument to only return requested length of data.
        if(req_len <= sm->size){
          // There is enough contiguous data to return
          rdata->ptr = sm->buf_write_start - remaining_elements;
          sm->read_closed_read_size = req_len;
          sm->read_closed_new_head = (((uint8_t*)rdata->ptr) + sm->read_closed_read_size) - sm->buf_write_start;
        }else{
          // There is NOT enough contiguous data to satisfy the request
          rdata->ptr = sm->buf_write_start - remaining_elements;
          sm->read_closed_read_size = sm->size;
          sm->read_closed_new_head = (((uint8_t*)rdata->ptr) + sm->read_closed_read_size) - sm->buf_write_start;
        }
      }
    }
    rdata->size = sm->read_closed_read_size;

    if(status == SM_SUCCESS){
      // read is open
      sm->read_open = true;
    }
  }
  return status;
}

/*
 * SM_READ_NOT_STARTED - nothing happened
 * SM_SUCCESS - read closed
 */
sm_status_t smem_read_close (smem_t* sm){
  sm_status_t status = SM_SUCCESS;
  // take semaphore
  if(!(sm->read_open)){
    status = SM_READ_NOT_STARTED;
  }else{
    // read open flag must be set false on a call to close
    sm->read_open = false;
    // make cached values permanent
    sm->head = sm->read_closed_new_head;
    sm->size -= sm->read_closed_read_size;

    status = SM_SUCCESS;
  }
  return status;
}

