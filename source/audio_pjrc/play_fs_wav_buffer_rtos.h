#ifndef play_fs_wav_buffer_h_
#define play_fs_wav_buffer_h_



#include <play_fs_wav_buffer_rtos.h>
#include <play_fs_wav_parser.h>
#include "fsl_common.h"
#include "fsl_pint.h"

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "semphr.h"
#include "ff.h"
#include "config.h"
#include "stream_memory_rtos.h"

// TODO: Use this RAM in a more clever manner?
// If this is set to 1, only one instance of "AudioPlayFsWav" can exist at a time.
#define USE_SRAMX_AUDIO_BUFFER (0U)

// TODO: Check why everything runs when WAV_BUFFER_FS_READ_CHUNK_SIZE is 512, but not when it is 4096.
//#define WAV_BUFFER_FS_READ_CHUNK_SIZE 4096
//#define WAV_BUFFER_FS_READ_CHUNK_SIZE 1024
#define WAV_BUFFER_FS_READ_CHUNK_SIZE 512
#define WAV_BUFFER_TRIGGER_LEVEL_BYTES 4 // 4 bytes = 2 byte left channel, 2 byte right channel.

#if (defined(USE_SRAMX_AUDIO_BUFFER) && (USE_SRAMX_AUDIO_BUFFER > 0U))
//#define WAV_BUFFER_SIZE_BYTES  (0x3000-1)
//#define SRAMX_ADDRESS (0x04001000)
//#define WAV_BUFFER_SIZE_BYTES  (0x8000-1)
//#define SRAMX_ADDRESS (0x04000000)
#define WAV_BUFFER_SIZE_BYTES  (0x4000-1)
#define SRAMX_ADDRESS (0x0001C000)
#else
// The number of bytes removed from the buffer during an update() call.
// The 4* multiple assumes 2 channel audio data is stored on buffer.
#define WAV_BUFFER_MAX_READ_MSG_LEN    (4*AUDIO_BLOCK_SAMPLES) // bytes
//
#define WAV_BUFFER_MAX_WRITE_MSG_LEN    (4*AUDIO_BLOCK_SAMPLES) // bytes
// The number of audio blocks on the buffer.
#define WAV_BUFFER_NUM_MSGS    40 // TODO: Determine why Audio task was crashing when this number was 100.
// The total number of audio bytes to be buffered.
#define WAV_BUFFER_SIZE_BYTES  WAV_BUFFER_MAX_WRITE_MSG_LEN * WAV_BUFFER_NUM_MSGS // bytes
#endif

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

enum wav_buffer_message 
{
  WAV_BUFFER_MSG_DATA = 0,
  WAV_BUFFER_MSG_EOF,
  WAV_BUFFER_MSG_LOOPING,
  WAV_BUFFER_EMPTY,
  WAV_BUFFER_MSG_ERR
};

#define FS_WAV_BUFFER_EVENT_QUEUE_SIZE 4

typedef enum
{
  FS_WAV_BUFFER_STATE_STOPPED = 0,
  FS_WAV_BUFFER_STATE_READING
} fs_wav_buffer_state_type_t;

typedef enum
{
  FS_WAV_BUFFER_EVENT_NONE = 0,
  FS_WAV_BUFFER_EVENT_START,
  FS_WAV_BUFFER_EVENT_STOP
} fs_wav_buffer_event_type_t;

typedef struct
{
  fs_wav_buffer_event_type_t type;
  char filename[256];
  bool loop;
} fs_wav_buffer_event_t;

typedef struct
{
  uint8_t* data;
  size_t size;
} fs_wav_buffer_return_t;


class AudioPlayFsWavBufferRTOS {
public:
  AudioPlayFsWavBufferRTOS(void) { begin_buffer(); }
    void begin_buffer(void);
    void pretask_init(void);
    bool start_buffer(const char *filename, bool loop = false);
    bool stop_buffer(void);
    bool fill_buffer(void);
    bool buffer_is_idle(void);
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
    fs_wav_buffer_return_t get_from_buffer(size_t data_len);
#else
    size_t get_from_buffer(void* data, size_t data_len);
#endif
    void release_from_buffer();
    static bool fill_all_buffers(void);
    static void pretask_init_all_buffers(void);
    void get_stream_params(uint8_t &state_play, uint32_t &sample_rate){
      parser.get_stream_params(state_play, sample_rate);
    }
protected:
    static AudioPlayFsWavBufferRTOS *buffer_first_update; // init: static in cpp
    AudioPlayFsWavBufferRTOS *buffer_next_update;         // init: begin_buffer()
    void start(fs_wav_buffer_event_t &event);
    void stop();
private:
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
#if (defined(USE_SRAMX_AUDIO_BUFFER) && (USE_SRAMX_AUDIO_BUFFER > 0U))
    uint8_t* smr_array = (uint8_t*) SRAMX_ADDRESS;     // Use SRAMX
#else
    uint8_t smr_array[ WAV_BUFFER_SIZE_BYTES+1 ];       // init: begin_buffer()
#endif
    // allocate stream memory
    smem_rtos_t smr;
#else
    uint8_t data[ WAV_BUFFER_FS_READ_CHUNK_SIZE ];

    // Audio Buffer
#if (defined(USE_SRAMX_AUDIO_BUFFER) && (USE_SRAMX_AUDIO_BUFFER > 0U))
    uint8_t* sbuf_array = (uint8_t*) SRAMX_ADDRESS;     // Use SRAMX
#else
    uint8_t sbuf_array[ WAV_BUFFER_SIZE_BYTES+1 ];       // init: begin_buffer()
#endif
    StaticStreamBuffer_t sbuf_struct;                  // init: begin_buffer()
    StreamBufferHandle_t sbuf_handle;                  // init: begin_buffer()
#endif // (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))


    // Vars that should only be accessed from fill_buffer;
    FIL wav_file;                                      // init: begin_buffer(); only access from fill_buffer();
    bool looping;                                      // init: begin_buffer(); only access from fill_buffer();
    AudioPlayFsWavParser parser;                     // init: begin_buffer(); only access from fill_buffer();
    fs_wav_buffer_state_type_t buffer_state;         // init: begin_buffer(); only access from fill_buffer();

    // Event
    uint8_t equeue_array[FS_WAV_BUFFER_EVENT_QUEUE_SIZE*sizeof(fs_wav_buffer_event_t)];
    StaticQueue_t equeue_struct;
    QueueHandle_t equeue_handle;
};

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif // play_fs_wav_buffer_h_
