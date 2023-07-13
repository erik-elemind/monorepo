/*
 * data_log_buffer.h
 *
 *  Created on: Apr 22, 2022
 *      Author: DavidWang
 */

#ifndef DATA_LOG_DATA_LOG_BUFFER_H_
#define DATA_LOG_DATA_LOG_BUFFER_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "FreeRTOSConfig.h" // needed for TickType_t and configASSERT


#ifdef __cplusplus
extern "C" {
#endif


class DLBuffer
{
private:

protected:
  DLBuffer(size_t buf_size) : buf_(NULL), buf_size_(buf_size) , buf_off_(0) {
  }

public:
  uint8_t* buf_ = NULL;
  const size_t buf_size_;
  size_t buf_off_ = 0;

  DLBuffer(void* buf, size_t buf_size) : buf_((uint8_t*)buf), buf_size_(buf_size) , buf_off_(0) {
    configASSERT( buf != NULL );
  }

  void reset(){
    buf_off_ = 0;
  }

  void add(void* data, size_t data_size){
    configASSERT( (buf_off_ + data_size) <= buf_size_ );
    memcpy( buf_+buf_off_, data, data_size );
    buf_off_ += data_size;
  }

  void add(uint8_t data){
    add((void*) &data, 1);
  }
};


#ifdef __cplusplus
}
#endif

#endif /* DATA_LOG_DATA_LOG_BUFFER_H_ */
