/*
 * message_memory.h
 *
 *  Created on: May 4, 2022
 *      Author: DavidWang
 */

/*
 * WARNING: The following code is BUGGY and is NOT ready for general use.
 * Date: May 31st, 2022
 */

#ifndef MEMORY_MANAGER_MESSAGE_MEMORY_H_
#define MEMORY_MANAGER_MESSAGE_MEMORY_H_

#include <stddef.h>
#include "stream_memory.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
  // constant values set by mmem_init()
  smem_t sm;

  // the number of messages
  size_t num_msgs;

  // write_open(), write_close() cached values
  void* current_write_ptr;

  // read_open(), read_close() cached values
  // nothing
}mmem_t;


#define MM_HDR_TYPE size_t

void mmem_init (mmem_t* mm, void* buf, size_t buf_cap, size_t max_msg_block_size);
void mmem_reset(mmem_t* mm);

sm_status_t mmem_write_open (mmem_t* mm, size_t requested_lenm, smem_return_buf_t* rdata);
sm_status_t mmem_write_close(mmem_t* mm);

sm_status_t mmem_read_open(mmem_t* mm, smem_return_buf_t* rdata);
sm_status_t mmem_read_close (mmem_t* mm);

#ifdef __cplusplus
}
#endif



#endif /* MEMORY_MANAGER_MESSAGE_MEMORY_H_ */
