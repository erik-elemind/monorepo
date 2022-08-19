/*
 * circular_storage.h
 *
 *  Created on: Aug 26, 2021
 *      Author: DavidWang
 */

#ifndef UTILS_CIRCULAR_BUFFER_H_
#define UTILS_CIRCULAR_BUFFER_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum{
  CBUF_SUCCESS = 0,
  CBUF_ERROR = 1,
  CBUF_NO_SPACE,
  CBUF_ADD_NOTHING,
  CBUF_ADD_TO_TAIL,
  CBUF_ADD_TO_ZERO,
  CBUF_REMOVE_NOTHING,
} circular_buffer_return_t;

typedef struct
{
  uint8_t* buffer;
  size_t capacity;
  int head; // oldest index with data
  int tail; // next index in which to store data
  int tail_skipped; // indices equal-to and after this index are unused.
} circular_buffer_t;


void circular_buffer_init(circular_buffer_t* circbuf, size_t capacity, uint8_t* buffer);
void circular_buffer_empty(circular_buffer_t* circbuf);
circular_buffer_return_t circular_buffer_fits_tail(circular_buffer_t* circbuf, uint8_t* data, size_t len);
circular_buffer_return_t circular_buffer_add_tail (circular_buffer_t* circbuf, uint8_t* data, size_t len);
circular_buffer_return_t circular_buffer_add_strategy(circular_buffer_t* circbuf, uint8_t* data, size_t len, circular_buffer_return_t add_strategy);
uint8_t* circular_buffer_get_head(circular_buffer_t* circbuf);
circular_buffer_return_t circular_buffer_remove_head(circular_buffer_t* circbuf, size_t len);

#ifdef __cplusplus
}
#endif


#endif /* UTILS_CIRCULAR_BUFFER_H_ */
