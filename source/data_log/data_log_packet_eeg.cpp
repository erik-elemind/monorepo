/*
 * data_log_packet_eeg.cpp
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
#include "eeg_datatypes.h"
#include "compression.h"
#include "data_log_eeg_comp.h"

static inline void
stream_eeg( unsigned long sample_number, int32_t eeg_channels[] ){
  // print the data
#if (defined(ENABLE_DATA_LOG_STREAM_EEG) && (ENABLE_DATA_LOG_STREAM_EEG == 1U))
  // send the data to the visualization
  LOGV("data_log_eeg", "%lu %ld %ld %ld",
  (unsigned long) sample_number, (long) eeg_channels[EEG_FP1], (long) eeg_channels[EEG_FPZ], (long) eeg_channels[EEG_FP2] );
#endif
}


static void
stream_eeg( ads129x_frontal_sample *f_sample ){
  // print the data
  stream_eeg( f_sample->sample_number, f_sample->eeg_channels );
}


#if (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_IGNORE))

void data_log_eeg_init(){}
void data_log_eeg_reset(){}
void data_log_eeg(ads129x_frontal_sample *f_sample){
  stream_eeg( f_sample );
}
void handle_eeg_data(uint8_t* buf, size_t size){}

#elif (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_BASIC))

void data_log_eeg_init(){}
void data_log_eeg_reset(){}

void data_log_eeg( ads129x_frontal_sample *f_sample ){
  // print the data
  stream_eeg( f_sample );

#define EEG_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(f_sample->eeg_channels) )
  uint8_t* scratch = (uint8*) DL_MALLOC(EEG_BUFFER_SIZE);
  DLBuffer dlbuf(scratch, EEG_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  // copy packet type
  dlbuf.add(DLPT_EEG_DATA);
  // copy timestamp
  dlbuf.add(&(f_sample->sample_number), sizeof(f_sample->sample_number));
  // copy data
  dlbuf.add(&(f_sample->eeg_channels[0]), sizeof(f_sample->eeg_channels));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef EEG_BUFFER_SIZE
}

void handle_eeg_data(uint8_t* buf, size_t size){}

#elif (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_PACKED))

class EEG_DLBufferPacked : public DLBufferPacked{
public:
  EEG_DLBufferPacked(void* buf, size_t buf_size, data_log_packet_t packet_type, size_t sample_count_target, TickType_t xTicksToWait) :
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
    ads129x_frontal_sample *f_sample = (ads129x_frontal_sample *) data;
    // save off the remaining channels
    for ( uint8_t i=0; i<MAX_NUM_EEG_CHANNELS; i++ ) {
      uint8_t* eeg_channel_as_bytes = (uint8_t*) &(f_sample->eeg_channels[i]);
      // add eeg data to the buffer
      DLBuffer::add(eeg_channel_as_bytes, (size_t)3);
    }
  }
};
uint8_t* g_eeg_arr[EEG_PACK_BUFFER_SIZE];
EEG_DLBufferPacked g_eeg_buf(g_eeg_arr,EEG_PACK_BUFFER_SIZE,
    DLPT_EEG_DATA_PACKED,EEG_PACK_NUM_SAMPLES_TO_SEND,
    portMAX_DELAY);

void data_log_eeg_init(){}

void data_log_eeg_reset(){
  g_eeg_buf.reset();
}

void data_log_eeg(ads129x_frontal_sample *f_sample) {
  // print the data
  stream_eeg( f_sample );

  g_eeg_buf.add(f_sample->sample_number, f_sample);
}

void handle_eeg_data(uint8_t* buf, size_t size){}

// (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_IGNORE))
// (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_BASIC))
// (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_PACKED))
#endif

/*****************************************************************************/
// General EEG Compression functions - Used in Offline and Online Compression

static void
eeg_comp_init_header(comp_params_t* comp_params) {
  comp_params->compression_version = 1;
  comp_params->frame_size = EEG_SAMPLE_FRAME_SIZE;
  comp_params->keep_num_coeff = (int) (0.3 * comp_params->frame_size); // 0.3
  comp_params->q_bits = 10;
  memset(comp_params->wavelet_name, 0, sizeof(comp_params->wavelet_name));
  memcpy(comp_params->wavelet_name, "CDF9/7", 7);
  comp_params->wavelet_level = 3;
  configASSERT(comp_params->frame_size <= EEG_SAMPLE_FRAME_SIZE_MAX);
}

static size_t
eeg_comp_write_header(comp_params_t* comp_params, uint8_t* scratch, size_t scratch_size) {
  configASSERT(scratch_size >= DLPT_COMP_HEADER_SIZE)

  size_t scratch_offset = 0;

  // save the packet marker
  add_to_buffer(scratch, scratch_size, scratch_offset, DLPT_EEG_COMP_HEADER);

  // add the compressed header
  int comp_size = compress_header(comp_params, &(scratch[scratch_offset]), scratch_size - scratch_offset);
  scratch_offset += comp_size;

  return scratch_offset;
}

void
eeg_comp_init(eeg_comp_t* eeg_comp){
  eeg_comp_init_header(&(eeg_comp->comp_params));
}

void
eeg_comp_reset(eeg_comp_t* eeg_comp) {
  eeg_comp->first_eeg_header = true;
  eeg_comp->first_eeg_sample_number = true;
}

// TODO: Add comment that this is being called w/int32_t values that are implicitly promoted to float
static bool
eeg_comp_add_data(eeg_comp_frame_buf_t *eeg_buf, size_t frame_size, float data) {
  eeg_buf->buffer[eeg_buf->index] = data;
  eeg_buf->index ++;
  if (eeg_buf->index >= frame_size) {
    eeg_buf->index = 0;
    return true; // return true that the buffer is full
  }
  return false;
}

static size_t
eeg_comp_compress(eeg_comp_t* eeg_comp, uint8_t chnum, uint32_t sample_num, uint8_t* scratch_buffer, size_t scratch_size) {
  float* frame_buf = eeg_comp->bufs.eegs[chnum].buffer;
  size_t frame_size = eeg_comp->comp_params.frame_size;

  // Reset scratch buffer
  size_t scratch_offset = 0;

  // save the packet marker
  add_to_buffer(scratch_buffer, scratch_size, scratch_offset, DLPT_EEG_COMP_FRAME);
  // save the sample number
  add_to_buffer(scratch_buffer, scratch_size, scratch_offset, &sample_num, sizeof(uint32_t));
  // save the eeg channel number
  add_to_buffer(scratch_buffer, scratch_size, scratch_offset, &chnum, sizeof(uint8_t));

  // compress the frame
  uint8_t* scratch_buffer_ptr = scratch_buffer + scratch_offset;// + scratch_padding;
  size_t scratch_buffer_remain_size = scratch_buffer + scratch_size - scratch_buffer_ptr;
  size_t scratch_buffer_used_size = compress_frame(
      frame_buf, // frame_buf
      frame_size, // frame_size
      eeg_comp->comp_params.wavelet_level, // wavelet_level
      eeg_comp->comp_params.q_bits,
      eeg_comp->comp_params.keep_num_coeff,
      scratch_buffer_ptr, // comp_buf
      scratch_buffer_remain_size // comp_buf_size
      );

  scratch_offset += scratch_buffer_used_size;

  return scratch_offset;
}


static bool
eeg_comp_add_data_and_write_helper(eeg_comp_t* eeg_comp, int32_t eeg_channels[], size_t chnum){
  int32_t               eeg        = eeg_channels[chnum];
  eeg_comp_frame_buf_t* frame_buf  = &(eeg_comp->bufs.eegs[chnum]);
  size_t                frame_size = eeg_comp->comp_params.frame_size;
  size_t scratch_size = EEG_COMP_BUFFER_SIZE(frame_size);

  if (eeg_comp_add_data(frame_buf, frame_size, eeg)) {
    // Buffer is full, compress and write it
    // Get scratch buffer
    uint8_t* scratch_buffer = eeg_comp->get_buffer(scratch_size);
    if(scratch_buffer != NULL){
      size_t scratch_offset = eeg_comp_compress(eeg_comp, chnum, eeg_comp->eeg_sample_number, scratch_buffer, scratch_size);
      eeg_comp->write_buffer(scratch_buffer, scratch_offset);
      return true;
    }
  }
  return false;
}

void
eeg_comp_add_data_and_write(eeg_comp_t* eeg_comp, uint32_t sample_number, int32_t eeg_channels[]){
  if (eeg_comp->first_eeg_header) {
    // Get scratch buffer
    uint8_t* scratch_buffer = eeg_comp->get_buffer(DLPT_COMP_HEADER_SIZE);
    if(scratch_buffer != NULL){
      size_t scratch_offset = eeg_comp_write_header(&(eeg_comp->comp_params), scratch_buffer, DLPT_COMP_HEADER_SIZE);
      eeg_comp->write_buffer(scratch_buffer, scratch_offset);
      eeg_comp->first_eeg_header = false;
    }
  }

  if (eeg_comp->first_eeg_sample_number) {
    eeg_comp->first_eeg_sample_number = false;
    eeg_comp->eeg_sample_number = sample_number;
  }

  eeg_comp_add_data_and_write_helper(eeg_comp, eeg_channels, 0);

  eeg_comp_add_data_and_write_helper(eeg_comp, eeg_channels, 1);

  if(eeg_comp_add_data_and_write_helper(eeg_comp, eeg_channels, 2)){
    // this is the final write so reset the sample_number
    eeg_comp->first_eeg_sample_number = true;
  }
}








#if (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_COMP))

#if 0
static eeg_comp_t g_eeg_comp_online;

static uint8_t* get_buffer(size_t scratch_size){
  return (uint8_t*) dl_malloc_if_file_ready(scratch_size);
}

static void write_buffer(uint8_t* buffer, size_t size){
  send_data(buffer, size, 0); // portMAX_DELAY
  // The scratch_buffer will be freed in the data log task
}

void data_log_eeg_init() {
  eeg_comp_init(&g_eeg_comp_online);
  g_eeg_comp_online.get_buffer = get_buffer;
  g_eeg_comp_online.write_buffer = write_buffer;
  data_log_eeg_reset();
}

void data_log_eeg_reset() {
  eeg_comp_reset(&g_eeg_comp_online);
}

void
data_log_eeg(ads129x_frontal_sample *f_sample) {
  // print the data
  stream_eeg( f_sample );

  // write the eeg data
  eeg_comp_add_data_and_write(&g_eeg_comp_online, f_sample->sample_number, f_sample->eeg_channels);
}

void handle_eeg_data(uint8_t* buf, size_t size){}

#else

alignas(4) static uint8_t g_scratch_buffer[EEG_COMP_BUFFER_SIZE_MAX] = {0};
static eeg_comp_t g_eeg_comp_online;

static uint8_t* get_buffer(size_t scratch_size){
  return g_scratch_buffer;
}

static void write_buffer(uint8_t* buffer, size_t size){
  data_log_write(buffer, size);
  // The scratch_buffer will be freed in the data log task
}

void data_log_eeg_init() {
  eeg_comp_init(&g_eeg_comp_online);
  g_eeg_comp_online.get_buffer = get_buffer;
  g_eeg_comp_online.write_buffer = write_buffer;
  data_log_eeg_reset();
}

void data_log_eeg_reset() {
  eeg_comp_reset(&g_eeg_comp_online);
}

void data_log_eeg( ads129x_frontal_sample *f_sample ){
#define EEG_BUFFER_SIZE sizeof(ads129x_frontal_sample)
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(EEG_BUFFER_SIZE);
  size_t scratch_offset = 0;
  if(scratch==NULL) {return;}
  // copy f_sample
  add_to_buffer(scratch, EEG_BUFFER_SIZE, scratch_offset, f_sample, sizeof(ads129x_frontal_sample));
  // send
  send_data(scratch, scratch_offset, DATA_LOG_EEG_DATA, portMAX_DELAY);
#undef EEG_BUFFER_SIZE
}


void handle_eeg_data(uint8_t* buf, size_t size){
  configASSERT(size == sizeof(ads129x_frontal_sample));

  ads129x_frontal_sample *f_sample = (ads129x_frontal_sample *) buf;

  // print the data
  stream_eeg( f_sample );

  // write the eeg data
  eeg_comp_add_data_and_write(&g_eeg_comp_online, f_sample->sample_number, f_sample->eeg_channels);
}

#endif



#endif  //  (defined(LOG_EEG) && (LOG_EEG == PACKET_TYPE_COMP))


