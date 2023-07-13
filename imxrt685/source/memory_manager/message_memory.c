/*
 * message_memory.c
 *
 *  Created on: May 4, 2022
 *      Author: DavidWang
 */

/*
 * WARNING: The following code is BUGGY and is NOT ready for general use.
 * Date: May 31st, 2022
 */

#include <string.h>
#include "message_memory.h"


void mmem_init (mmem_t* mm, void* buf, size_t buf_cap, size_t max_msg_block_size){
  smem_init(&(mm->sm), buf, buf_cap, max_msg_block_size);
  mmem_reset(mm);
}

void mmem_reset(mmem_t* mm){
  mm->num_msgs = 0;
}

sm_status_t mmem_write_open (mmem_t* mm, size_t requested_len, smem_return_buf_t* rdata){
  // get a stream buffer of size (header and requested_len).
  sm_status_t status = smem_write_open(&(mm->sm), sizeof(MM_HDR_TYPE) + requested_len, rdata);
  void* ptr = rdata->ptr;
  if(status == SM_SUCCESS){
    mm->current_write_ptr = ptr;
    // write the header
    smem_write(&(mm->sm), ptr, sizeof(MM_HDR_TYPE));
    // write the body
    smem_write(&(mm->sm), ptr+sizeof(MM_HDR_TYPE), requested_len);
    // return the requested_len without the header.
    rdata->ptr = ((uint8_t*)ptr) + sizeof(MM_HDR_TYPE);
    rdata->size = requested_len;
  }
  return status;
}

sm_status_t mmem_write_close(mmem_t* mm){
  // close the write
  sm_status_t status = smem_write_close(&(mm->sm));
  if(status == SM_SUCCESS){
    // save the number of bytes written to the message header
    MM_HDR_TYPE len = (MM_HDR_TYPE)(mm->sm.write_total_len) - sizeof(MM_HDR_TYPE);
    memmove(mm->current_write_ptr, &len, sizeof(MM_HDR_TYPE));
    // reset the current write ptr;
    mm->current_write_ptr = NULL;
    //increment the msg count
    mm->num_msgs++;
  }
  return status;
}

sm_status_t mmem_read_open(mmem_t* mm, smem_return_buf_t* rdata){
  sm_status_t status = smem_read_open(&(mm->sm), sizeof(MM_HDR_TYPE), false, rdata);
  smem_read_close(&(mm->sm));
  if(status != SM_SUCCESS){
    return status;
  }
  return smem_read_open(&(mm->sm), rdata->size, false, rdata);
}

sm_status_t mmem_read_close (mmem_t* mm){
  sm_status_t status = smem_read_close(&(mm->sm));
  //decrement the msg count
  mm->num_msgs--;
  return status;
}
