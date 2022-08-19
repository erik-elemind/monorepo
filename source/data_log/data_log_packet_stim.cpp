/*
 * data_log_packet_stim.cpp
 *
 *  Created on: Apr 23, 2022
 *      Author: DavidWang
 */

#include <data_log_packet.h>
#include "data_log_buffer_packed.h"
#include "data_log_internal.h"
#include "data_log.h"
#include "loglevels.h"
#include "data_log_buffer.h"

static
inline void stream_stim_amp(unsigned long sample_number, float stim_amp){
#if (defined(ENABLE_DATA_LOG_STREAM_STIM) && (ENABLE_DATA_LOG_STREAM_STIM > 0U))
  LOGV("data_log_stim_amp","%lu %f", (unsigned long) sample_number, (float) stim_amp);
#endif
}

#if (defined(LOG_STIM_AMP) && (LOG_STIM_AMP == PACKET_TYPE_IGNORE))

void data_log_stim_reset(){}
void data_log_stimulus_amplitude(unsigned long sample_number, float stim_amp){
  stream_stim_amp(sample_number,stim_amp);
}

#elif (defined(LOG_STIM_AMP) && (LOG_STIM_AMP == PACKET_TYPE_BASIC))

void data_log_stim_reset(){}

// log stimulus amplitude
void data_log_stimulus_amplitude(unsigned long sample_number, float stim_amp){
  stream_stim_amp(sample_number,stim_amp);

  // constraint stim amp to a single byte
  uint8_t stim_amp_uint8;
  if(stim_amp<=0){
    stim_amp_uint8 = 0;
  }else if(stim_amp >=1){
    stim_amp_uint8 = 255;
  }else{
    stim_amp_uint8 = stim_amp*255;
  }

#define STIM_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(uint8_t))
  uint8_t* scratch = (uint8*) DL_MALLOC(STIM_BUFFER_SIZE);
  size_t offset = 0;
  // copy packet type
  dlbuf.add(DLPT_STIM_AMP);
  // copy sample number
  dlbuf.add(&sample_number, sizeof(sample_number));
  // copy inst amp
  dlbuf.add(&stim_amp_uint8, sizeof(stim_amp_uint8));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef STIM_BUFFER_SIZE
}

#elif (defined(LOG_STIM_AMP) && (LOG_STIM_AMP == PACKET_TYPE_PACKED))

class Stim_DLBufferPacked : public DLBufferPacked{
public:
  Stim_DLBufferPacked(void* buf, size_t buf_size, data_log_packet_t packet_type, size_t sample_count_target, TickType_t xTicksToWait) :
    DLBufferPacked(buf,buf_size,packet_type,sample_count_target,xTicksToWait){
  }
protected:
  void* mem_alloc(size_t bytes){
    return dl_malloc_if_file_ready(bytes);
  }
  void send_data(TickType_t xTicksToWait){
    ::send_data(this, xTicksToWait);
  }
  void add_data(void* data){
    // save inst amp
    DLBuffer::add(data, sizeof(uint8_t));
  }
};
uint8_t* g_stim_arr[STIM_AMP_PACK_BUFFER_SIZE];
Stim_DLBufferPacked g_stim_buf(g_stim_arr,STIM_AMP_PACK_BUFFER_SIZE,
    DLPT_STIM_AMP_PACKED,STIM_AMP_PACK_NUM_SAMPLES_TO_SEND,
    portMAX_DELAY);

#endif

void data_log_stim_reset(){
  g_stim_buf.reset();
}

// log stimulus amplitude
void data_log_stimulus_amplitude(unsigned long sample_number, float stim_amp){
  stream_stim_amp(sample_number,stim_amp);

  // constraint stim amp to a single byte
  uint8_t stim_amp_uint8;
  if(stim_amp<=0){
    stim_amp_uint8 = 0;
  }else if(stim_amp >=1){
    stim_amp_uint8 = 255;
  }else{
    stim_amp_uint8 = stim_amp*255;
  }

  g_stim_buf.add(sample_number, &stim_amp_uint8);
}
