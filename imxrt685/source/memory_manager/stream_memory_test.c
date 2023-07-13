/*
 * stream_memory_test.c
 *
 *  Created on: Apr 24, 2022
 *      Author: DavidWang
 */

#include <stdbool.h>
#include "stream_memory_test.h"
#include "stream_memory.h"
#include "loglevels.h"

static inline void print_buf1(char* name, uint8_t* buf, size_t size){
  printf("%s [", name);
  for(int i=0; i<size; i++){
    if(buf[i]==-1){
      printf("X,");
    }else{
      printf("%d,",buf[i]);
    }
  }
  printf("]\n");
}

static inline void print_buf2(char* name, smem_return_buf_t* data){
  print_buf1(name, data->ptr, data->size);
}

static inline char* status_name(sm_status_t status){
  switch(status){
  case SM_SUCCESS:             return "SM_SUCCESS";
  case SM_ERROR:               return "SM_ERROR";
  case SM_WRITE_IN_PROGRESS:   return "SM_WRITE_IN_PROGRESS";
  case SM_WRITE_NOT_STARTED:   return "SM_WRITE_NOT_STARTED";
  case SM_WRITE_NO_SPACE:      return "SM_WRITE_NO_SPACE";
  case SM_WRITE_NOTHING:       return "SM_WRITE_NOTHING";
  case SM_WRITE_BW_TAIL_END:   return "SM_WRITE_BW_TAIL_END";
  case SM_WRITE_BW_TAIL_HEAD:  return "SM_WRITE_BW_TAIL_HEAD";
  case SM_WRITE_TO_ZERO:       return "SM_WRITE_TO_ZERO";
  case SM_WRITE_OUT_OF_BOUNDS: return "SM_WRITE_OUT_OF_BOUNDS";
  case SM_READ_IN_PROGRESS:    return "SM_READ_IN_PROGRESS";
  case SM_READ_NOT_STARTED:    return "SM_READ_NOT_STARTED";
  case SM_READ_NOTHING:        return "SM_READ_NOTHING";
  default: return "SM_STATUS_UNDEFINED";
  }
}

static inline void print_status(char* name, sm_status_t status){
  printf("%s %s\n",name,status_name(status));
}

static inline void print_sm_struct(smem_t *sm){
  printf("head: %d, tail: %d, tail_skip: %d\n", sm->head, sm->tail, sm->tail_skipped);
  printf("size: %d, write_open: %d, read_open: %d\n", sm->size, sm->write_open, sm->read_open);
}

static inline void print_success(bool success){
  if(success){
    printf("SUCCESS!\n");
  }else{
    printf("FAIL!\n");
  }
}

/*
 * Check that two buffers are equivalent.
 * Values equal to 255 in exp_buf are skipped.
 * Returns true if there is a match.
 */
static inline bool check_buffers(uint8_t* act_buf, uint8_t* exp_buf, size_t size){
  bool match = true;
  for(int i=0; i<size; i++){
    if(exp_buf[i]!=255){
      // only check for match is values are positive
      match &= (act_buf[i]==exp_buf[i]);
    }
  }
  return match;
}

static void stream_memory_basic_test(){
  printf("stream_memory_basic_test\n");
#define MAX_READ_BLOCK_SIZE (3)
#define READ_BLOCK_SIZE (3)
#define WRITE_BLOCK_SIZE (5)
#define BUFFER_SIZE (MAX_READ_BLOCK_SIZE+WRITE_BLOCK_SIZE+WRITE_BLOCK_SIZE)

  uint8_t buffer[BUFFER_SIZE] = {0};
  smem_t sm;
  smem_init (&sm, buffer, BUFFER_SIZE, MAX_READ_BLOCK_SIZE);

  bool success = true;

  sm_status_t status;
  {
    printf("======================\n");
    printf("write\n");
    // write result = X,X,X, [0,1,2,X,X],X,X,X,X,X
    smem_return_buf_t rdata;
    smem_write_open (&sm, WRITE_BLOCK_SIZE, &rdata);
    uint8_t* ptr = rdata.ptr;
    ptr[0] = 0;
    ptr[1] = 1;
    ptr[2] = 2;
    status = smem_write(&sm, ptr, 3);
    print_status("sm_write_commit:", status);
    status = smem_write_close(&sm);
    print_status("sm_write_close:", status);
    print_buf1("buffer: ",buffer,BUFFER_SIZE);
    print_sm_struct(&sm);

    // programmatically check for match
    uint8_t exp[] = {-1,-1,-1, 0, 1, 2,-1,-1,-1,-1,-1,-1,-1};
    success &= check_buffers(buffer, exp, BUFFER_SIZE);
    print_success(success);
  }

  {
    printf("======================\n");
    printf("write\n");
    // write result = X,X,X, 0,1,2,[3,4,5,6,7],X,X
    smem_return_buf_t rdata;
    smem_write_open (&sm, WRITE_BLOCK_SIZE, &rdata);
    uint8_t* ptr = rdata.ptr;
    ptr[0] = 3;
    ptr[1] = 4;
    ptr[2] = 5;
    ptr[3] = 6;
    ptr[4] = 7;
    status = smem_write(&sm, ptr, 5);
    print_status("sm_write_commit:", status);
    status = smem_write_close(&sm);
    print_status("sm_write_close:", status);
    print_buf1("buffer: ",buffer,BUFFER_SIZE);
    print_sm_struct(&sm);

    // programmatically check for match
    uint8_t exp[] = {-1,-1,-1, 0, 1, 2, 3, 4, 5, 6, 7,-1,-1};
    success &= check_buffers(buffer, exp, BUFFER_SIZE);
    print_success(success);
  }

  {
    printf("======================\n");
    printf("\n\nread\n");
    // read result = X,X,X, [X,X,X],3,4,5,6,7,X,X
    // returned: 0,1,2,
    smem_return_buf_t data;
    status = smem_read_open(&sm, READ_BLOCK_SIZE, false, &data);
    smem_read_close(&sm);
    print_status("sm_read_open:", status);
    print_buf2("readbuf: ",&data);
    print_buf1("buffer : ",buffer,BUFFER_SIZE);
    print_sm_struct(&sm);

    // programmatically check for match
    uint8_t exp[] = {-1,-1,-1,-1,-1,-1, 3, 4, 5, 6, 7,-1,-1};
    success &= check_buffers(buffer, exp, BUFFER_SIZE);
    uint8_t exp_read[] = {0,1,2};
    success &= check_buffers(data.ptr, exp_read, READ_BLOCK_SIZE);
    print_success(success);
  }

  {
    printf("======================\n");
    printf("write\n");
    // write result = X,X,X, X,X,X,3,4,5,6,7,X,X
    smem_return_buf_t rdata;
    status = smem_write_open (&sm, WRITE_BLOCK_SIZE, &rdata);
    uint8_t* ptr = rdata.ptr;
    if(ptr == NULL){

    }
    status = smem_write(&sm, ptr, 0);
    print_status("sm_write_commit:", status);
    status = smem_write_close(&sm);
    print_status("sm_write_close:", status);
    print_buf1("buffer: ",buffer,BUFFER_SIZE);
    print_sm_struct(&sm);

    // programmatically check for match
    uint8_t exp[] = {-1,-1,-1,-1,-1,-1, 3, 4, 5, 6, 7,-1,-1};
    success &= check_buffers(buffer, exp, BUFFER_SIZE);
    print_success(success);
  }

  {
    printf("======================\n");
    printf("\n\nread\n");
    // read result = X,X,X, X,X,X,[X,X,X],6,7,X,X
    // returned: 3,4,5,
    smem_return_buf_t data;
    status = smem_read_open(&sm, READ_BLOCK_SIZE, false, &data);
    smem_read_close(&sm);
    print_status("sm_read_open:", status);
    print_buf2("readbuf: ",&data);
    print_buf1("buffer: ",buffer,BUFFER_SIZE);
    print_sm_struct(&sm);

    // programmatically check for match
    uint8_t exp[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1, 6, 7,-1,-1};
    success &= check_buffers(buffer, exp, BUFFER_SIZE);
    uint8_t exp_read[] = {3,4,5};
    success &= check_buffers(data.ptr, exp_read, READ_BLOCK_SIZE);
    print_success(success);
  }

  {
    printf("======================\n");
    printf("write\n");
    // write result = X,X,X, [8,9,10,11,12],X,6,7,X,X
    smem_return_buf_t rdata;
    status = smem_write_open (&sm, WRITE_BLOCK_SIZE, &rdata);
    uint8_t* ptr = rdata.ptr;
    ptr[0] = 8;
    ptr[1] = 9;
    ptr[2] = 10;
    ptr[3] = 11;
    ptr[4] = 12;
    status = smem_write(&sm, ptr, 5);
    print_status("sm_write_commit:", status);
    status = smem_write_close(&sm);
    print_status("sm_write_close:", status);
    print_buf1("buffer: ",buffer,BUFFER_SIZE);
    print_sm_struct(&sm);

    // programmatically check for match
    uint8_t exp[] = {-1,-1,-1, 8, 9,10,11,12,-1, 6, 7,-1,-1};
    success &= check_buffers(buffer, exp, BUFFER_SIZE);
    print_success(success);
  }

  {
    printf("======================\n");
    printf("\n\nread\n");
    // read result = X,[6,7, 8],9,10,11,12,X,X,X,X,X
    // returned: 6,7,8,
    smem_return_buf_t data;
    status = smem_read_open(&sm, READ_BLOCK_SIZE, false, &data);
    smem_read_close(&sm);
    print_status("sm_read_open:", status);
    print_buf2("readbuf: ",&data);
    print_buf1("buffer: ",buffer,BUFFER_SIZE);
    print_sm_struct(&sm);

    // programmatically check for match
    uint8_t exp[] = {-1,-1,-1,-1, 9,10,11,12,-1,-1,-1,-1,-1};
    success &= check_buffers(buffer, exp, BUFFER_SIZE);
    uint8_t exp_read[] = {6,7,8};
    success &= check_buffers(data.ptr, exp_read, READ_BLOCK_SIZE);
    print_success(success);
  }



#undef READ_BLOCK_SIZE
#undef WRITE_BLOCK_SIZE
#undef BUFFER_SIZE
}


void stream_memory_test(){

  stream_memory_basic_test();

}
