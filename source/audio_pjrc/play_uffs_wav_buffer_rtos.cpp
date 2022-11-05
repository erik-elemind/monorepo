/*
 * play_uffs_wav_buffer.c
 *
 *  Created on: Sep 27, 2020
 *      Author: David Wang
 */


#include "play_uffs_wav_buffer_rtos.h"
#include "wavbuf.h"
#include "ff.h"
#include "fatfs_utils.h"
#include "loglevels.h"
#include "utils.h"
#include "wavbuf.h"
#include "micro_clock.h"

//static const char *TAG = "play_uffs_wav_buffer_rtos";   // Logging prefix for this module

AudioPlayUffsWavBufferRTOS* AudioPlayUffsWavBufferRTOS::buffer_first_update = NULL;

void AudioPlayUffsWavBufferRTOS::begin_buffer(void)
{
  //
  // WARNING: This code is called before main(), due to ResetISR(), so no printf() here.
  //
  // initialize variables
  memset(&wav_file, 0, sizeof(wav_file));
  looping = false;
  buffer_state = UFFS_WAV_BUFFER_STATE_STOPPED;
  parser.begin_parser();

  // update list
  if (buffer_first_update == NULL) {
    buffer_first_update = this;
  } else {
    AudioPlayUffsWavBufferRTOS *p;
    for (p=buffer_first_update; p->buffer_next_update; p = p->buffer_next_update) ;
    p->buffer_next_update = this;
  }
  buffer_next_update = NULL;
}

void AudioPlayUffsWavBufferRTOS::pretask_init(void)
{
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
  // create the stream buffer
  smem_rtos_init (&smr, smr_array, WAV_BUFFER_SIZE_BYTES, WAV_BUFFER_MAX_READ_MSG_LEN);
#else
  // create the audio buffer
//  memset(sbuf_array, 0, WAV_BUFFER_SIZE_BYTES); // memset happens in xStreamBufferCreateStatic
  sbuf_handle = xStreamBufferCreateStatic(WAV_BUFFER_SIZE_BYTES, WAV_BUFFER_TRIGGER_LEVEL_BYTES, sbuf_array, &sbuf_struct);
#endif

  // create the event queue
  equeue_handle = xQueueCreateStatic(UFFS_WAV_BUFFER_EVENT_QUEUE_SIZE,sizeof(uffs_wav_buffer_event_t),equeue_array,&equeue_struct);
  vQueueAddToRegistry(equeue_handle, "play_uffs_wav_buffer_event_queue");
}

bool AudioPlayUffsWavBufferRTOS::start_buffer(const char *filename, bool loop)
{
  // stop the previous buffer fill oepration
  stop_buffer();

  uffs_wav_buffer_event_t event;
  event.type = UFFS_WAV_BUFFER_EVENT_START;
  strcpy(event.filename,filename);
  event.loop = loop;
  xQueueSend(equeue_handle, &event, portMAX_DELAY);

#if (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK))
  wavbuf_task_wakeup();
#endif
  return true;
}

bool AudioPlayUffsWavBufferRTOS::stop_buffer(void)
{
  uffs_wav_buffer_event_t event;
  event.type = UFFS_WAV_BUFFER_EVENT_STOP;
  event.filename[0] = '\0';
  event.loop = false;
  xQueueSend(equeue_handle, &event, portMAX_DELAY);

#if (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK))
  wavbuf_task_wakeup();
#endif
  return true;
}

void AudioPlayUffsWavBufferRTOS::start(uffs_wav_buffer_event_t &event){
  // close any previously open file
  stop();
  // try and open the file
  FRESULT result = f_open(&wav_file, event.filename, FA_READ);
  if (FR_OK == result) {
    looping = event.loop;
    // Empty the audio stream, in case the audio stream format
    // changes on the next wav file to be played.
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
    smem_rtos_reset(&smr);
#else
    xStreamBufferReset(sbuf_handle);
#endif
    parser.start_parser();
    buffer_state = UFFS_WAV_BUFFER_STATE_READING;
  }
}

void AudioPlayUffsWavBufferRTOS::stop(){
  // close any previously open file
  if(f_is_open(&wav_file)){
    f_close(&wav_file);
    looping = false;
    parser.stop_parser();
    buffer_state = UFFS_WAV_BUFFER_STATE_STOPPED;
  }
}

bool AudioPlayUffsWavBufferRTOS::buffer_is_idle(){
  return UFFS_WAV_BUFFER_STATE_STOPPED == buffer_state;
}

bool AudioPlayUffsWavBufferRTOS::fill_buffer(void)
{
  // Get the next event, but do not wait for it
  uffs_wav_buffer_event_t event;
  while ( xQueueReceive(equeue_handle, &event, 0) == pdPASS ) {
    switch(event.type){
      case UFFS_WAV_BUFFER_EVENT_START:
        start(event);
        break;

      case UFFS_WAV_BUFFER_EVENT_STOP:
        stop();
        break;

      default:
        // do nothing
        break;
    }
  }

  // Read, parse, and send audio data from file over audio file stream.
  /*size_t sbuf_spaces_avail = 0;*/
  if ( f_is_open(&wav_file) && buffer_state == UFFS_WAV_BUFFER_STATE_READING
      /* && ((sbuf_spaces_avail=xStreamBufferSpacesAvailable(sbuf_handle)) >= WAV_BUFFER_UFFS_READ_CHUNK_SIZE)*/) {

//    uint64_t start_us = micros();
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
    UINT data_len = 0;
    // TODO: Choose a better error result value
    FRESULT result = FR_INVALID_PARAMETER;
    smem_return_buf_t rdata;
    smem_rtos_write_open (&smr, WAV_BUFFER_UFFS_READ_CHUNK_SIZE, &rdata, portMAX_DELAY); // TODO: Check port max delay
    void* data = rdata.ptr;
    // TODO: do something if we can't grab memory!
    if(data != NULL){
      result = f_read(&wav_file, data, WAV_BUFFER_UFFS_READ_CHUNK_SIZE, &data_len);
    }
    // LOGV("play_uffs_wav_buffer","read us: %lu", (uint32_t)(micros()-start_us));
    if(FR_OK != result){
      stop();
       // TODO: Handle return error of smem_rtos_write_close?
      smem_rtos_write_close(&smr);
      return false;
    }

    // Some data was read, so try to parse it.
    if(data_len > 0){
      // Parse the data into the outgoing stream.
      parser.parse( (uint8_t*)data, data_len, (void*)&smr );
    }

    sm_status_t write_status = smem_rtos_write_close(&smr);
    if(write_status == SM_SUCCESS){
      result = FR_OK;
    }
#else
    UINT data_len = 0;
    FRESULT result;
    result = f_read(&wav_file, data, WAV_BUFFER_UFFS_READ_CHUNK_SIZE, &data_len);

    // LOGV("play_uffs_wav_buffer","read us: %lu", (uint32_t)(micros()-start_us));
    if(FR_OK != result){
      stop();
      return false;
    }

    // Some data was read, so try to parse it.
    if(data_len > 0){
      // Parse the data into the outgoing stream.
      parser.parse( data, data_len, &sbuf_handle );
    }
#endif

    // Less than the expected data was read, which indicates the file reached its end.
    if (data_len < WAV_BUFFER_UFFS_READ_CHUNK_SIZE){
//      LOGV("play_uffs_wav_buffer","wavfile data_len: %u, buf avail spaces: %u",data_len, xStreamBufferSpacesAvailable(sbuf_handle));

      // Check the looping variable to see if the buffer should be played again.
      if(looping){
        // Seek back to the start of the file
//        uint64_t start_us = micros();
        result = f_lseek(&wav_file, parser.get_audio_data_offset());
        if(FR_OK != result){
          stop();
          return false;
        }
//        LOGV("play_uffs_wav_buffer","seek us: %lu", (uint32_t)(micros()-start_us));
        // reset the parser
        parser.start_parser_at_audio_data_offset();
      }else{
        // The file has ended, and we are NOT looping.
        stop();
      }
    }

  }

  // Return TRUE if we are still in the active reading-file state.
  return (buffer_state == UFFS_WAV_BUFFER_STATE_READING);
}


#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))

uffs_wav_buffer_return_t AudioPlayUffsWavBufferRTOS::get_from_buffer(size_t data_len)
{
  uffs_wav_buffer_return_t return_value = {0};

  smem_return_buf_t sm_data;
  /*sm_status_t read_status = */smem_rtos_read_open(&smr, data_len, false, &sm_data, 0);
  return_value.data = (uint8_t*)sm_data.ptr;
  return_value.size = sm_data.size;

  return return_value;
}

#else


size_t AudioPlayUffsWavBufferRTOS::get_from_buffer(void* data, size_t data_len)
{
  size_t xReceivedBytes;
  xReceivedBytes = xStreamBufferReceive( sbuf_handle, data, data_len, 0 ); // portMAX_DELAY
  return xReceivedBytes;
}


#endif

void  AudioPlayUffsWavBufferRTOS::release_from_buffer()
{
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
  smem_rtos_read_close(&smr);
#else
#endif
}


// Fills the buffer for all wav file objects.
// Returns TRUE if any wav file object still needs its buffer filled.
bool AudioPlayUffsWavBufferRTOS::fill_all_buffers(void){
  AudioPlayUffsWavBufferRTOS *p;
  // load additional wav files into buffer
  bool active = false;
  for (p = AudioPlayUffsWavBufferRTOS::buffer_first_update; p; p = p->buffer_next_update) {
    active |= p->fill_buffer();
  }
  return active;
}

void AudioPlayUffsWavBufferRTOS::pretask_init_all_buffers(void){
  AudioPlayUffsWavBufferRTOS *p;
  for (p = AudioPlayUffsWavBufferRTOS::buffer_first_update; p; p = p->buffer_next_update) {
    p->pretask_init();
  }
}
