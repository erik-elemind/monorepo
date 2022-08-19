//#include <Arduino.h>
#include "AudioCompat.h"
#include "mp3.h"
#include "utils.h"

#if (defined(AUDIO_ENABLE_MP3) && (AUDIO_ENABLE_MP3 > 0U))

static TaskHandle_t g_mp3_task_handle = NULL;
static StaticStreamBuffer_t sbuf_struct;                  
static StreamBufferHandle_t sbuf_handle;

extern Mp3 mp3output;

static const char *TAG = "mp3";   // Logging prefix for this module

Mp3 mp3output;

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

Mp3::~Mp3() {
  MP3FreeDecoder(decoder);
}

Mp3::Mp3() : AudioStream(0, NULL) {
  decoder = MP3InitDecoder();
}

void Mp3::stop() {
  playing = false;
  f_close(&file);
}

void Mp3::fill_inbuf() {
  // Only refill if we are below threshold.
  if (inbuf_left >= BUFFER_LOWER_THRESH) {
    return;
  }

  // Move bytes back to beginning: helix doesn't support circular
  // buffer.
  memmove(inbuf, inbuf_ptr, inbuf_left);
  inbuf_ptr = inbuf;

  UINT bytes_read;
  if (FR_OK != f_read(&file, &inbuf[inbuf_left], (sizeof(inbuf) - inbuf_left), &bytes_read)) {
    stop();
    return;
  }
  
  inbuf_left += bytes_read;
}

void Mp3::open(const char* filename) {
  if (f_open(&file, filename, FA_READ) != FR_OK) {
    LOGE(TAG, "Could not open mp3: %s", filename);
    return;
  }
  decode_success = false;
}

void Mp3::play() {
  if (!playing) {
    inbuf_ptr = inbuf;
    inbuf_left = 0;
    fill_inbuf();
    
    // Find the sample rate and number of channels
    int offset = MP3FindSyncWord(inbuf_ptr, inbuf_left);
    if (offset < 0) {
      LOGE(TAG, "Bad mp3 file");
      return;
    }
    inbuf_left -= offset;
    inbuf_ptr += offset;

    if (MP3GetNextFrameInfo(decoder, &frame, inbuf_ptr)) {
      LOGE(TAG, "Could not read mp3 frame info");
      return;
    }
    
    if (frame.samprate != 22050 && frame.samprate != 44100) {
      LOGE(TAG, "MP3 decoder only supports 22050 & 44100 hz, found %i hz", frame.bitrate);
      return;
    }

    if (frame.nChans != 1 && frame.nChans != 2) {
      LOGE(TAG, "MP3 decoder only supports 1 or 2 channels, found %i", frame.nChans);
    }
    
    LOGI(TAG, "Decoding at %i hz %i channel(s)", frame.samprate, frame.nChans);

    playing = true;
    mp3_task_wakeup();
  }
}

int Mp3::decode_frame() {
  static unsigned char outbuf[OUTPUT_BUFFER_SIZE * sizeof(short)];
  
  fill_inbuf();

  // Advance to next frame
  int offset = MP3FindSyncWord(inbuf_ptr, inbuf_left);
  if (offset < 0) {
    if (decode_success && loop) {
      // We may have reached the end of the file.  Try looping.
      decode_success = false;

      if (FR_OK != f_rewind(&file)) {
	    return -1;
      }

      // Main loop will call decode_frame again.
      return 0;
    }
      
    return -1;
  }
  inbuf_left -= offset;
  inbuf_ptr += offset;
  
  // Decode the frame.
  MP3DecInfo *mp3DecInfo = (MP3DecInfo *)decoder;
  if (MP3Decode(decoder, &inbuf_ptr, (int*)&inbuf_left, (short*)outbuf, 0) < 0) {
    decode_success = false;
    return -1;
  }
  unsigned toRead = (mp3DecInfo->nGrans * mp3DecInfo->nGranSamps * mp3DecInfo->nChans) * sizeof(short);
  unsigned outbuf_left = 0;
  unsigned char* outbuf_ptr = outbuf;
  
  // Convert to mono, 22050.    
  unsigned short* short_ptr = (unsigned short*)outbuf_ptr;
  size_t outp = 0;
  unsigned skip;
  if (frame.nChans == 2 && frame.samprate == 44100) {
    skip = 3;
  } else if (frame.nChans == 2 && frame.samprate == 22050) {
    skip = 1;
  } else if (frame.nChans == 1 && frame.samprate == 44100) {
    skip = 1;
  } else if (frame.nChans == 1 && frame.samprate == 22050) {
    skip = 0;
  }
  if (skip) {
    for(size_t i = 0; i < (toRead / sizeof(short));) {
      short_ptr[outp++] = short_ptr[i++];
      outbuf_left+=2;
      i += skip;
    }
  } else {
    outbuf_left = toRead;
  }

  // Streaming by AUDIO_BLOCK_SIZE*sizeof(short)  seems to work much better than sending the whole
  // block at once.
  while(outbuf_left > 0) {
    size_t written = xStreamBufferSend( sbuf_handle, outbuf_ptr, MIN(TRANSFER_SIZE, outbuf_left), portMAX_DELAY);
//    LOGV("mp3","mp3 send: %d",(int)written);
    outbuf_ptr += written;
    outbuf_left -= written;
  }

  decode_success = true;

  return 0;
}

bool Mp3::is_playing() {
  return playing;
}

void Mp3::update()
{
  audio_block_t *block = NULL;
  block = allocate();

//  LOGV("mp3","mp3 update");
  if (block) {
      // If there is no audio data available, do NOT wait.
    xStreamBufferReceive( sbuf_handle, ((unsigned char*)block->data), TRANSFER_SIZE,  0);
//    LOGV("mp3","mp3 bytes: %d",(int)num_bytes);
    transmit(block);
    release(block);
  }
}

bool Mp3::is_idle(void)
{
  return !playing;
}

#define MP3_BUFFER_TRIGGER_LEVEL_BYTES (4)

#if 1
#define MP3_BUFFER_SIZE_BYTES (AUDIO_BLOCK_SAMPLES * sizeof(short) * 5)
static uint8_t sbuf_array[ MP3_BUFFER_SIZE_BYTES];
#else
#define WAV_BUFFER_SIZE_BYTES  (0x4000-1)
#define SRAMX_ADDRESS (0x04000000)
#endif

#define MP3_BUFFER_SIZE_BYTES WAV_BUFFER_SIZE_BYTES
static uint8_t *sbuf_array = (uint8_t*) SRAMX_ADDRESS;

void
mp3_pretask_init(void) {
  g_mp3_task_handle = xTaskGetCurrentTaskHandle();
  sbuf_handle = xStreamBufferCreateStatic(MP3_BUFFER_SIZE_BYTES, MP3_BUFFER_TRIGGER_LEVEL_BYTES, sbuf_array, &sbuf_struct);
}

void
mp3_task_wakeup() {
  // Wake up the sleeping task
  xTaskNotify( g_mp3_task_handle,
               0x0,  // ignored
               eNoAction );
}

void mp3_task(void *ignored) {
  mp3_pretask_init();
  
  uint32_t notification_value;

  while (1) {
//    LOGV("mp3","task while loop");
    if (!mp3output.is_playing()){
      // Wait for a start notification
      xTaskNotifyWait( 0,                    // Clear no bits on entry
                       UINT32_MAX,            // Clear all bits on exit
                       &notification_value,  // ignored
                       portMAX_DELAY );
      
      xStreamBufferReset(sbuf_handle);
//      LOGV("mp3","RESET!");
    }

    // While playing, we will block in xStreamBufferSend.
//    int retval = 0;
    if( (/*retval = */mp3output.decode_frame()) ) {
      mp3output.stop();
    }
//    LOGV("mp3","task decode_frame retval: %d", retval);
  }
}

#endif // AUDIO_ENABLE_MP3
