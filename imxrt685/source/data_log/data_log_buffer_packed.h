/*
 * data_log_buffer_packed.h
 *
 *  Created on: Apr 22, 2022
 *      Author: DavidWang
 */

#ifndef DATA_LOG_DATA_LOG_BUFFER_PACKED_H_
#define DATA_LOG_DATA_LOG_BUFFER_PACKED_H_

#include "data_log_packet.h"
#include "data_log_buffer.h"
#include "FreeRTOSConfig.h" // needed for TickType_t and configASSERT

#ifdef __cplusplus
extern "C" {
#endif


class DLBufferPacked : public DLBuffer
{
private:

protected:
  virtual void* mem_alloc(size_t bytes) = 0;
  virtual void send_data(TickType_t xTicksToWait) = 0;
  virtual void add_data(void* data) = 0;

public:
  const data_log_packet_t packet_type_;
  const size_t sample_count_target_;
  const TickType_t send_wait_ticks_;
  size_t sample_count_;
  size_t prev_sample_number_;

  DLBufferPacked(void* buf, size_t buf_size, data_log_packet_t packet_type, size_t sample_count_target, TickType_t xTicksToWait) :
    DLBuffer(buf_size), packet_type_(packet_type), sample_count_target_(sample_count_target), send_wait_ticks_(xTicksToWait), sample_count_(0), prev_sample_number_(0) {
    configASSERT(sample_count_target != 0);
  }

  void reset(){
    DLBuffer::reset();
    sample_count_ = 0;
    prev_sample_number_ = 0;
  }

  void add(unsigned long sample_number, void* data){
    // allocate memory
    // send
    if ( (sample_count_ == sample_count_target_) ||
         ((prev_sample_number_+1 != sample_number) && sample_count_>0) ){
      // send data
      send_data(send_wait_ticks_);
      // reinitialize values
      buf_ = NULL;
      buf_off_ = 0;
      sample_count_ = 0;
    }
    if(buf_ == NULL){
      buf_ = (uint8_t*) mem_alloc(buf_size_);
      if(buf_==NULL) {return;}
    }
    // save off the sample number for use in the next call to pack_eeg.
    prev_sample_number_ = sample_number;

    // If the sample counter is 0, save off:
    // 1. The packet marker
    // 2. The first sample number
    if (sample_count_ == 0){
      // save the packet marker
      DLBuffer::add(packet_type_);
      // save the sample number
      DLBuffer::add(&(sample_number), sizeof(sample_number));
    }
    // save off the data
    add_data(data);
    // increase the sample count
    sample_count_++;
  }

};

#ifdef __cplusplus
}
#endif


#endif /* DATA_LOG_DATA_LOG_BUFFER_PACKED_H_ */
