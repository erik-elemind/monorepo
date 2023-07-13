/*
 * data_log_packet_inst.cpp
 *
 *  Created on: Apr 23, 2022
 *      Author: DavidWang
 */

#include "data_log_packet.h"
#include "data_log_buffer_packed.h"
#include "data_log_internal.h"
#include "data_log.h"
#include "loglevels.h"
#include "data_log_buffer.h"


static
inline void stream_inst_amp_phs(unsigned long sample_number, float instAmp, float instPhs){
#if (defined(ENABLE_DATA_LOG_STREAM_INST) && (ENABLE_DATA_LOG_STREAM_INST > 0U))
  LOGV("data_log_inst_amp_phs","%lu %f %f", (unsigned long) sample_number, (float)instAmp, (float)instPhs);
#endif
}

#if (defined(LOG_INST_AMP_PHS) && (LOG_INST_AMP_PHS == PACKET_TYPE_IGNORE))

void data_log_inst_init(){}
void data_log_inst_reset(){}
void data_log_inst_amp_phs(uint32_t sample_number, float instAmp, float instPhs) {
  stream_inst_amp_phs(sample_number,instAmp,instPhs);
}
void handle_inst_data(uint8_t* buf, size_t size){}

#elif (defined(LOG_INST_AMP_PHS) && (LOG_INST_AMP_PHS == PACKET_TYPE_BASIC))

void data_log_inst_init(){}
void data_log_inst_reset(){}

// log amplitude and phase
void data_log_inst_amp_phs(uint32_t sample_number, float instAmp, float instPhs)
{
  stream_inst_amp_phs(sample_number,instAmp,instPhs);
#define INST_AMP_PHS_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(instAmp) + sizeof(instPhs) )
  uint8_t* scratch = (uint8*) DL_MALLOC(INST_AMP_PHS_BUFFER_SIZE);
  DLBuffer dlbuf(scratch, INST_AMP_PHS_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_INST_AMP_PHS);
  // copy sample number
  dlbuf.add(&sample_number, sizeof(sample_number));
  // copy inst amp
  dlbuf.add(&instAmp, sizeof(instAmp));
  // copy inst phase
  dlbuf.add(&instPhs, sizeof(instPhs));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef INST_AMP_PHS_BUFFER_SIZE
}

void handle_inst_data(uint8_t* buf, size_t size){}


#elif (defined(LOG_INST_AMP_PHS) && (LOG_INST_AMP_PHS == PACKET_TYPE_PACKED))

class Inst_DLBufferPacked : public DLBufferPacked{
public:
  Inst_DLBufferPacked(void* buf, size_t buf_size, data_log_packet_t packet_type, size_t sample_count_target, TickType_t xTicksToWait) :
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
    DLBuffer::add((float*)data, sizeof(float));
    // save inst phase
    DLBuffer::add(((float*)data)+1, sizeof(float));
  }
};
uint8_t* g_inst_arr[INST_PACK_BUFFER_SIZE];
Inst_DLBufferPacked g_inst_buf(g_inst_arr,INST_PACK_BUFFER_SIZE,
    DLPT_INST_AMP_PHS_PACKED,INST_PACK_NUM_SAMPLES_TO_SEND,
    portMAX_DELAY);

void data_log_inst_init(){}

void data_log_inst_reset(){
  g_inst_buf.reset();
}

// log amplitude and phase
void data_log_inst_amp_phs(uint32_t sample_number, float instAmp, float instPhs)
{
  stream_inst_amp_phs(sample_number,instAmp,instPhs);
  void* data[2] = {&instAmp, &instPhs};
  g_inst_buf.add(sample_number, data);
}

void handle_inst_data(uint8_t* buf, size_t size){}

// (defined(PACKET_TYPE_BASIC) && (LOG_INST_AMP_PHS == PACKET_TYPE_IGNORE))
// (defined(PACKET_TYPE_BASIC) && (LOG_INST_AMP_PHS == PACKET_TYPE_BASIC))
// (defined(LOG_INST_AMP_PHS) && (LOG_INST_AMP_PHS == PACKET_TYPE_PACKED))
#endif



/*****************************************************************************/
// General INST Compression functions

static void
inst_amp_comp_init_header(comp_params_t* comp_params) {
  comp_params->compression_version = 1;
  comp_params->frame_size = INST_SAMPLE_FRAME_SIZE;
  comp_params->keep_num_coeff = (int) (0.3 * comp_params->frame_size); // 0.3
  comp_params->q_bits = 10;
  memset(comp_params->wavelet_name, 0, sizeof(comp_params->wavelet_name));
  memcpy(comp_params->wavelet_name, "CDF9/7", 7);
  comp_params->wavelet_level = 3;
  configASSERT(comp_params->frame_size <= INST_SAMPLE_FRAME_SIZE_MAX);
}

static void
inst_phs_comp_init_header(comp_params_t* comp_params) {
  comp_params->compression_version = 1;
  comp_params->frame_size = INST_SAMPLE_FRAME_SIZE;
  comp_params->keep_num_coeff = (int) (0.5 * comp_params->frame_size); // 0.3
  comp_params->q_bits = 10;
  memset(comp_params->wavelet_name, 0, sizeof(comp_params->wavelet_name));
  memcpy(comp_params->wavelet_name, "CDF9/7", 7);
  comp_params->wavelet_level = 3;
  configASSERT(comp_params->frame_size <= INST_SAMPLE_FRAME_SIZE_MAX);
}

static size_t
inst_comp_write_header(comp_params_t* comp_params, data_log_packet_t packet_type, uint8_t* scratch, size_t scratch_size) {
  configASSERT(scratch_size >= DLPT_COMP_HEADER_SIZE)

  size_t scratch_offset = 0;

  // save the packet marker
  add_to_buffer(scratch, scratch_size, scratch_offset, packet_type);

  // add the compressed header
  int comp_size = compress_header(comp_params, &(scratch[scratch_offset]), scratch_size - scratch_offset);
  scratch_offset += comp_size;

  return scratch_offset;
}

void
inst_comp_init(inst_comp_t* inst_comp){
  inst_amp_comp_init_header(&(inst_comp->comp_params.iamp));
  inst_phs_comp_init_header(&(inst_comp->comp_params.iphs));
}

void
inst_comp_reset(inst_comp_t* inst_comp) {
  inst_comp->first_inst_header = true;
  inst_comp->first_inst_sample_number = true;
}

// TODO: Add comment that this is being called w/int32_t values that are implicitly promoted to float
static bool
inst_comp_add_data(inst_comp_frame_buf_t *inst_buf, size_t frame_size, float data) {
  inst_buf->buffer[inst_buf->index] = data;
  inst_buf->index ++;
  if (inst_buf->index >= frame_size) {
    inst_buf->index = 0;
    return true; // return true that the buffer is full
  }
  return false;
}

static size_t
inst_comp_compress(inst_comp_t* inst_comp, uint8_t chnum, uint32_t sample_num, data_log_packet_t packet_type, uint8_t* scratch_buffer, size_t scratch_size) {
  float* frame_buf = inst_comp->bufs.arr[chnum].buffer;
  comp_params_t *comp_param = &(inst_comp->comp_params.arr[chnum]);
  size_t frame_size = comp_param->frame_size;

  // Reset scratch buffer
  size_t scratch_offset = 0;

  // save the packet marker
  add_to_buffer(scratch_buffer, scratch_size, scratch_offset, packet_type);
  // save the sample number
  add_to_buffer(scratch_buffer, scratch_size, scratch_offset, &sample_num, sizeof(uint32_t));
//  // save the inst channel number
//  add_to_buffer(scratch_buffer, scratch_size, scratch_offset, &chnum, sizeof(uint8_t));

  // compress the frame
  uint8_t* scratch_buffer_ptr = scratch_buffer + scratch_offset;// + scratch_padding;
  size_t scratch_buffer_remain_size = scratch_buffer + scratch_size - scratch_buffer_ptr;
  size_t scratch_buffer_used_size = compress_frame(
      frame_buf, // frame_buf
      frame_size, // frame_size
      comp_param->wavelet_level, // wavelet_level
      comp_param->q_bits,
      comp_param->keep_num_coeff,
      scratch_buffer_ptr, // comp_buf
      scratch_buffer_remain_size // comp_buf_size
      );

  scratch_offset += scratch_buffer_used_size;

  return scratch_offset;
}


static bool
inst_comp_add_data_and_write_helper(inst_comp_t* inst_comp, float inst, size_t chnum, data_log_packet_t packet_type){
  inst_comp_frame_buf_t* frame_buf  = &(inst_comp->bufs.arr[chnum]);
  size_t                frame_size = inst_comp->comp_params.arr[chnum].frame_size;
  size_t scratch_size = INST_COMP_BUFFER_SIZE(frame_size);

  if (inst_comp_add_data(frame_buf, frame_size, inst)) {
    // Buffer is full, compress and write it
    // Get scratch buffer
    uint8_t* scratch_buffer = inst_comp->get_buffer(scratch_size);
    if(scratch_buffer != NULL){
      size_t scratch_offset = inst_comp_compress(inst_comp, chnum, inst_comp->inst_sample_number, packet_type, scratch_buffer, scratch_size);
      inst_comp->write_buffer(scratch_buffer, scratch_offset);
      return true;
    }
  }
  return false;
}

void
inst_comp_add_data_and_write(inst_comp_t* inst_comp, inst_amp_phs_t* inst_amp_phs){
  if (inst_comp->first_inst_header) {
    // Write inst amplitude header
    // Get scratch buffer
    {
      uint8_t* scratch_buffer = inst_comp->get_buffer(DLPT_COMP_HEADER_SIZE);
      if(scratch_buffer != NULL){
        size_t scratch_offset = inst_comp_write_header(&(inst_comp->comp_params.iamp), DLPT_INST_AMP_COMP_HEADER, scratch_buffer, DLPT_COMP_HEADER_SIZE);
        inst_comp->write_buffer(scratch_buffer, scratch_offset);
        inst_comp->first_inst_header = false;
      }
    }
    // Write inst phase header
    // Get scratch buffer
    {
      uint8_t* scratch_buffer = inst_comp->get_buffer(DLPT_COMP_HEADER_SIZE);
      if(scratch_buffer != NULL){
        size_t scratch_offset = inst_comp_write_header(&(inst_comp->comp_params.iphs), DLPT_INST_PHS_COMP_HEADER, scratch_buffer, DLPT_COMP_HEADER_SIZE);
        inst_comp->write_buffer(scratch_buffer, scratch_offset);
        inst_comp->first_inst_header = false;
      }
    }
  }

  if (inst_comp->first_inst_sample_number) {
    inst_comp->first_inst_sample_number = false;
    inst_comp->inst_sample_number = inst_amp_phs->sample_number;
  }

  inst_comp_add_data_and_write_helper(inst_comp, inst_amp_phs->instAmp, INST_COMP_IAMP_CHNUM, DLPT_INST_AMP_COMP_FRAME);

  // We multiply phase by 1000 to get around some integer rounding issues internal to the compression algorithm.
  // TODO: Figure out what the integer rounding issue is in the compression algorithm.
  if(inst_comp_add_data_and_write_helper(inst_comp, inst_amp_phs->instPhs*1000, INST_COMP_IPHS_CHNUM, DLPT_INST_PHS_COMP_FRAME)){
    // this is the final write so reset the sample_number
    inst_comp->first_inst_sample_number = true;
  }
}

#if (defined(LOG_INST_AMP_PHS) && (LOG_INST_AMP_PHS == PACKET_TYPE_COMP))

#if 0
static inst_comp_t g_inst_comp_online;

static uint8_t* get_buffer(size_t scratch_size){
  return (uint8_t*) dl_malloc_if_file_ready(scratch_size);
}

static void write_buffer(uint8_t* buffer, size_t size){
  send_data(buffer, size, 0); // portMAX_DELAY
  // The scratch_buffer will be freed in the data log task
}

void data_log_inst_init() {
  inst_comp_init(&g_inst_comp_online);
  g_inst_comp_online.get_buffer = get_buffer;
  g_inst_comp_online.write_buffer = write_buffer;
  data_log_inst_reset();
}

void data_log_inst_reset() {
  inst_comp_reset(&g_inst_comp_online);
}

void
data_log_inst(ads129x_frontal_sample *f_sample) {
  // print the data
  stream_inst_amp_phs(inst_amp_phs->sample_number,inst_amp_phs->instAmp,inst_amp_phs->instPhs);

  // write the inst data
  inst_comp_add_data_and_write(&g_inst_comp_online, inst_amp_phs);
}

void handle_inst_data(uint8_t* buf, size_t size){}

#else

alignas(4) static uint8_t g_scratch_buffer[INST_COMP_BUFFER_SIZE_MAX] = {0};
static inst_comp_t g_inst_comp_online;

static uint8_t* get_buffer(size_t scratch_size){
  return g_scratch_buffer;
}

static void write_buffer(uint8_t* buffer, size_t size){
  data_log_write(buffer, size);
  // The scratch_buffer will be freed in the data log task
}

void data_log_inst_init() {
  inst_comp_init(&g_inst_comp_online);
  g_inst_comp_online.get_buffer = get_buffer;
  g_inst_comp_online.write_buffer = write_buffer;
  data_log_inst_reset();
}

void data_log_inst_reset() {
  inst_comp_reset(&g_inst_comp_online);
}

void data_log_inst_amp_phs(uint32_t sample_number, float instAmp, float instPhs){
#define INST_BUFFER_SIZE sizeof(inst_amp_phs_t)
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(INST_BUFFER_SIZE);
  size_t scratch_offset = 0;
  if(scratch==NULL) {return;}
#if 0
  // save sample number
  add_to_buffer(scratch, INST_BUFFER_SIZE, scratch_offset, &sample_number, sizeof(sample_number));
  // save inst amp
  add_to_buffer(scratch, INST_BUFFER_SIZE, scratch_offset, &instAmp, sizeof(instAmp));
  // save inst phs
  add_to_buffer(scratch, INST_BUFFER_SIZE, scratch_offset, &instPhs, sizeof(instPhs));
#endif
  inst_amp_phs_t datum = {sample_number , instAmp, instPhs};
  add_to_buffer(scratch, INST_BUFFER_SIZE, scratch_offset, &datum, sizeof(datum));
  // send
  send_data(scratch, scratch_offset, DATA_LOG_INST_DATA, portMAX_DELAY);
#undef INST_BUFFER_SIZE
}

void handle_inst_data(uint8_t* buf, size_t size){
  configASSERT(size == sizeof(inst_amp_phs_t));

  inst_amp_phs_t *inst_amp_phs = (inst_amp_phs_t *) buf;

  // print the data
  stream_inst_amp_phs(inst_amp_phs->sample_number,inst_amp_phs->instAmp,inst_amp_phs->instPhs);

  // write the inst data
  inst_comp_add_data_and_write(&g_inst_comp_online, inst_amp_phs);
}

#endif



#endif // (defined(LOG_INST) && (LOG_INST == PACKET_TYPE_COMP))


