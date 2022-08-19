/*
 * circular_storage.c
 *
 *  Created on: Aug 26, 2021
 *      Author: DavidWang
 */

#include <string.h>
#include "circular_buffer.h"


void circular_buffer_init(circular_buffer_t* circbuf, size_t capacity, uint8_t* buffer){
  circbuf->buffer = buffer;
  circbuf->capacity = capacity;
  circular_buffer_empty(circbuf);
}

void circular_buffer_empty(circular_buffer_t* circbuf){
  circbuf->head = -1;
  circbuf->tail = 0;
  circbuf->tail_skipped = -1;
}

circular_buffer_return_t circular_buffer_fits_tail(circular_buffer_t* circbuf, uint8_t* data, size_t len){
  // If len is 0, do nothing
  if (len <= 0){
    return CBUF_ADD_NOTHING;
  }

  if (circbuf->head < circbuf->tail){
    // head is before tail
    if ( len <= circbuf->capacity - circbuf->tail ){
      return CBUF_ADD_TO_TAIL;
    }else if( len <= circbuf->head ){
      return CBUF_ADD_TO_ZERO;
    }else{
      return CBUF_NO_SPACE;
    }
  }else if (circbuf->head > circbuf->tail){
    // head is after tail (data spans end of buffer)
    if ( len <= circbuf->head - circbuf->tail ){
      return CBUF_ADD_TO_TAIL;
    }else{
      return CBUF_NO_SPACE;
    }
  }

  // This happens if head == tail.
  return CBUF_NO_SPACE;
}

circular_buffer_return_t circular_buffer_add_tail (circular_buffer_t* circbuf, uint8_t* data, size_t len){
  circular_buffer_return_t add_strategy = circular_buffer_fits_tail( circbuf, data, len );
  return circular_buffer_add_strategy( circbuf, data, len, add_strategy );
}

circular_buffer_return_t circular_buffer_add_strategy(circular_buffer_t* circbuf, uint8_t* data, size_t len, circular_buffer_return_t add_strategy){

  switch(add_strategy){
  case CBUF_NO_SPACE || CBUF_ADD_NOTHING:
    return add_strategy;
  case CBUF_ADD_TO_TAIL:
    // data fits between tail and end of buffer
    memcpy( &(circbuf->buffer[circbuf->tail]), data, len );
    // update the tail pointer.
    circbuf->tail += len;
    return CBUF_SUCCESS;
  case CBUF_ADD_TO_ZERO:
    // data fits between start of buffer and head
    memcpy( &(circbuf->buffer[0]), data, len );
    // record the tail pointer that we are skipping
    circbuf->tail_skipped = circbuf->tail;
    // update the tail pointer.
    circbuf->tail = len;
    return CBUF_SUCCESS;
  default:
    // do nothing
    // return an error indicating the strategy was unrecognized
    return CBUF_ERROR;
  }

}

uint8_t* circular_buffer_get_head(circular_buffer_t* circbuf){
  if (circbuf->head == -1){
    // the buffer is empty
    return NULL;
  }
  return &(circbuf->buffer[circbuf->head]);
}

circular_buffer_return_t circular_buffer_remove_head(circular_buffer_t* circbuf, size_t len){
  // If len is 0, do nothing
  if (len <= 0){
    return CBUF_REMOVE_NOTHING;
  }

  circbuf->head += len;

  if ( circbuf->head >= circbuf->capacity ) {
    return CBUF_ERROR;
  }

  if (circbuf->tail_skipped == circbuf->head) {
    circbuf->head = 0;
  }

  // after removal, if head and tail are equal, it is because the buffer is empty.
  // restore the empty condition.
  if (circbuf->head == circbuf->tail){
    circular_buffer_empty(circbuf);
  }
  return CBUF_SUCCESS;
}





