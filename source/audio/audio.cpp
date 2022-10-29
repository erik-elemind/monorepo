/*
 * audio.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jul, 2020
 * Author:  David Wang
 * 
 * Basic documentation is available in this Google Doc:
 * https://docs.google.com/document/d/1uTERHHox20vDXZfLd5PRKSXwrYdiXV4cV4suHQzFgoU
 */

#include <stdlib.h>
#include <stdbool.h>

#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "loglevels.h"
#include "config.h"
#include "ble.h"
#include "audio.h"
#include "ssm2518.h"
#include "AudioPJRC.h"
#include "AudioStream.h"
#include "micro_clock.h"
#include "volume_scaling.h"
#include "memman_rtos.h"

#if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))

/*
 * The size of a void* pointer.
 * (aka the size of the processor-dependent pointer).
 * Used as an intermediate cast, to prevent warnings from stuffing
 * int/bool types directly into a void* pointer variable.
 */
#define VOID_STAR_CAST_TYPE uint32_t

#define AUDIO_EVENT_QUEUE_SIZE 50
#define AUDIO_EVENT_MEMORY_SIZE 600

#define AUDIO_IDLE_2_POWER_OFF_DELAY_MS 10000

static const char *TAG = "audio";   // Logging prefix for this module

#define AUDIO_FG_WAV_CHANNEL 0
#define AUDIO_BG_WAV_CHANNEL 1
#define AUDIO_MP3_CHANNEL 2
#define AUDIO_PINK_CHANNEL 3
#define AUDIO_SINE_CHANNEL 4

#define AUDIO_DATA_SIZE 18 // 14
static /*DMAMEM*/ audio_block_t AUDIO_DATA[AUDIO_DATA_SIZE];

AudioMixer5              mixerLeft;
AudioMixer5              mixerRight;

#if (defined(AUDIO_ENABLE_FG_WAV) && (AUDIO_ENABLE_FG_WAV > 0U))
AudioPlayUffsWav         wavFG;
AudioEffectFade          fade_fg_left(false);
AudioEffectFade          fade_fg_right(false);
AudioConnection          patchCord1(wavFG, 0, fade_fg_left , 0);
AudioConnection          patchCord2(wavFG, 1, fade_fg_right, 0);
AudioConnection          patchCord3(fade_fg_left, 0, mixerLeft , AUDIO_FG_WAV_CHANNEL);
AudioConnection          patchCord4(fade_fg_right, 0, mixerRight, AUDIO_FG_WAV_CHANNEL);
static float fgwav_gain = 0;
#endif

#if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
AudioPlayUffsWav         wavBG;
AudioEffectFade          bg_fade_left(false);
AudioEffectFade          bg_fade_right(false);
AudioConnection          patchCord5(wavBG, 0, bg_fade_left , 0);
AudioConnection          patchCord6(wavBG, 1, bg_fade_right, 0);
AudioConnection          patchCord7(bg_fade_left, 0, mixerLeft , AUDIO_BG_WAV_CHANNEL);
AudioConnection          patchCord8(bg_fade_right, 0, mixerRight, AUDIO_BG_WAV_CHANNEL);
static float bg_gain_script   = 0;
static float bg_gain_computed = 0;
static bool bg_mute = false;
#endif

#if (defined(AUDIO_ENABLE_MP3) && (AUDIO_ENABLE_MP3 > 0U))
AudioEffectFade          fade_mp3_left(false);
AudioEffectFade          fade_mp3_right(false);
AudioConnection          patchCord9(mp3output, 0, fade_mp3_left , 0);
AudioConnection          patchCord10(mp3output, 1, fade_mp3_right, 0);
AudioConnection          patchCord11(fade_mp3_left, 0, mixerLeft, AUDIO_MP3_CHANNEL);
AudioConnection          patchCord12(fade_mp3_right, 0, mixerRight, AUDIO_MP3_CHANNEL);
static float mp3_gain = 0;
#endif

#if (defined(AUDIO_ENABLE_PINK) && (AUDIO_ENABLE_PINK > 0U))
AudioSynthNoisePink      pink;
AudioEffectFade          pink_fade((false));
AudioConnection          patchCord13(pink, 0, pink_fade , 0);
AudioConnection          patchCord14(pink_fade , 0, mixerLeft , AUDIO_PINK_CHANNEL);
AudioConnection          patchCord15(pink_fade , 0, mixerRight, AUDIO_PINK_CHANNEL);
static float pink_gain_script   = 0;
static float pink_gain_computed = 0;
static bool pink_mute = false;
#endif

#if (defined(AUDIO_ENABLE_SINE) && (AUDIO_ENABLE_SINE > 0U))
AudioSynthWaveformSine   sineLow;
AudioSynthWaveformSine   sineHigh;
AudioConnection          patchCord16(sineLow , 0, mixerLeft , AUDIO_SINE_CHANNEL);
AudioConnection          patchCord17(sineHigh , 0, mixerRight, AUDIO_SINE_CHANNEL);
static float sine_gain = 0;
#endif

AudioOutputI2S           audioOutput;
AudioConnection          patchCord18(mixerLeft, 0, audioOutput, 0);
AudioConnection          patchCord19(mixerRight, 0, audioOutput, 1);

/*****************************************************************************/
// Gain Control
#define GAIN_KEEP (-1)

typedef enum{
  MUTE_FALSE=0,
  MUTE_TRUE,
  MUTE_KEEP,
} audio_mute_t;


//
// Task events:
//
typedef enum
{
  AUDIO_EVENT_ENTER_STATE,	// (used for state transitions)
  AUDIO_EVENT_POWER_ON,
  AUDIO_EVENT_POWER_OFF,
  AUDIO_EVENT_STOP,
  AUDIO_EVENT_PAUSE,
  AUDIO_EVENT_UNPAUSE,
  AUDIO_EVENT_POWER_OFF_TIMEOUT,

  AUDIO_EVENT_FGWAV_PLAY,
  AUDIO_EVENT_FGWAV_STOP,
  AUDIO_EVENT_FG_FADE_IN,
  AUDIO_EVENT_FG_FADE_OUT,

  AUDIO_EVENT_BGWAV_PLAY,
  AUDIO_EVENT_BGWAV_STOP,
  AUDIO_EVENT_BG_FADE_IN,
  AUDIO_EVENT_BG_FADE_OUT,

  AUDIO_EVENT_MP3_PLAY,
  AUDIO_EVENT_MP3_STOP,
  AUDIO_EVENT_MP3_FADE_IN,
  AUDIO_EVENT_MP3_FADE_OUT,

  AUDIO_EVENT_PINK_PLAY,
  AUDIO_EVENT_PINK_STOP,
  AUDIO_EVENT_PINK_FADE_IN,
  AUDIO_EVENT_PINK_FADE_OUT,

  AUDIO_EVENT_SINE_PLAY,
  AUDIO_EVENT_SINE_STOP,

  // stateless events
  AUDIO_EVENT_SET_MUTE,
  AUDIO_EVENT_SET_VOLUME,
  AUDIO_EVENT_SET_VOLUME_BLE,
  AUDIO_EVENT_SET_VOLUME_STEP,
  AUDIO_EVENT_VOLUME_UP,
  AUDIO_EVENT_VOLUME_DOWN,
  // software ISR
  AUDIO_EVENT_SOFTWARE_ISR_OCCURRED
} audio_event_type_t;

//
// Event user data:
//

typedef struct
{
  char filename[256];
  bool loop;
} play_file_data_t;

//
// State machine states:
//
typedef enum
{
  AUDIO_STATE_OFF,
  AUDIO_STATE_STANDBY,
} audio_state_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  audio_event_type_t type;
  void* user_data;
} audio_event_t;

//
// Global context data:
//
typedef struct
{
  audio_state_t state;
  TimerHandle_t audio_power_off_timer_handle;
  StaticTimer_t audio_power_off_timer_struct;

  uint8_t log_volume;
  uint8_t lin_volume;
  uint8_t log_volume_step;
  bool    mute;
} audio_context_t;

static audio_context_t ag_context;

// Global event queue and handler:
static uint8_t g_event_queue_array[AUDIO_EVENT_QUEUE_SIZE*sizeof(audio_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(audio_event_t *event);

// Global memory
static uint8_t* g_event_memory_buf[AUDIO_EVENT_MEMORY_SIZE];
static mm_rtos_t g_event_memory;

#define AUDIO_MALLOC(X) ((X*)mm_rtos_malloc(&g_event_memory,sizeof(X),portMAX_DELAY))

// Local non-thread safe event handlers:
static void audio_set_volume_int(uint8_t volume, bool update_ble);
static void audio_set_volume_step_int(uint8_t step);
static void audio_volume_up_int(void);
static void audio_volume_down_int(void);
static void audio_set_mute_int(bool);

// For logging and debug:
static const char *
audio_state_name(audio_state_t state)
{
  switch (state) {
    case AUDIO_STATE_OFF:          return "AUDIO_STATE_OFF";
    case AUDIO_STATE_STANDBY:      return "AUDIO_STATE_STANDBY";
    default:
      break;
  }
  return "AUDIO_STATE UNKNOWN";
}

static const char *
audio_event_type_name(audio_event_type_t event_type)
{
  switch (event_type) {
    case AUDIO_EVENT_ENTER_STATE:       return "AUDIO_EVENT_ENTER_STATE";
    case AUDIO_EVENT_POWER_ON:          return "AUDIO_EVENT_POWER_ON";
    case AUDIO_EVENT_POWER_OFF:         return "AUDIO_EVENT_POWER_OFF";
    case AUDIO_EVENT_STOP:              return "AUDIO_EVENT_STOP";
    case AUDIO_EVENT_PAUSE:             return "AUDIO_EVENT_PAUSE";
    case AUDIO_EVENT_UNPAUSE:           return "AUDIO_EVENT_UNPAUSE";
    case AUDIO_EVENT_POWER_OFF_TIMEOUT: return "AUDIO_EVENT_POWER_OFF_TIMEOUT";

    case AUDIO_EVENT_BGWAV_PLAY:        return "AUDIO_EVENT_BGWAV_PLAY";
    case AUDIO_EVENT_BGWAV_STOP:        return "AUDIO_EVENT_BGWAV_STOP";
    case AUDIO_EVENT_BG_FADE_IN:        return "AUDIO_EVENT_BG_FADE_IN";
    case AUDIO_EVENT_BG_FADE_OUT:       return "AUDIO_EVENT_BG_FADE_OUT";

    case AUDIO_EVENT_FGWAV_PLAY:        return "AUDIO_EVENT_FGWAV_PLAY";
    case AUDIO_EVENT_FGWAV_STOP:        return "AUDIO_EVENT_FGWAV_STOP";
    case AUDIO_EVENT_FG_FADE_IN:        return "AUDIO_EVENT_FG_FADE_IN";
    case AUDIO_EVENT_FG_FADE_OUT:       return "AUDIO_EVENT_FG_FADE_OUT";

    case AUDIO_EVENT_SINE_PLAY:         return "AUDIO_EVENT_SINE_PLAY";
    case AUDIO_EVENT_SINE_STOP:         return "AUDIO_EVENT_SINE_STOP";

    case AUDIO_EVENT_MP3_PLAY:          return "AUDIO_EVENT_MP3_PLAY";
    case AUDIO_EVENT_MP3_STOP:          return "AUDIO_EVENT_MP3_STOP";
    case AUDIO_EVENT_MP3_FADE_IN:       return "AUDIO_EVENT_MP3_FADE_IN";
    case AUDIO_EVENT_MP3_FADE_OUT:      return "AUDIO_EVENT_MP3_FADE_OUT";

    case AUDIO_EVENT_PINK_PLAY:         return "AUDIO_EVENT_PINK_PLAY";
    case AUDIO_EVENT_PINK_STOP:         return "AUDIO_EVENT_PINK_STOP";
    case AUDIO_EVENT_PINK_FADE_IN:      return "AUDIO_EVENT_PINK_FADE_IN";
    case AUDIO_EVENT_PINK_FADE_OUT:     return "AUDIO_EVENT_PINK_FADE_OUT";

    case AUDIO_EVENT_SET_MUTE:          return "AUDIO_EVENT_SET_MUTE";
    case AUDIO_EVENT_SET_VOLUME:        return "AUDIO_EVENT_SET_VOLUME";
    case AUDIO_EVENT_SET_VOLUME_BLE:    return "AUDIO_EVENT_SET_VOLUME_BLE";
    case AUDIO_EVENT_SET_VOLUME_STEP:   return "AUDIO_EVENT_SET_VOLUME_STEP";
    case AUDIO_EVENT_VOLUME_UP:         return "AUDIO_EVENT_VOLUME_UP";
    case AUDIO_EVENT_VOLUME_DOWN:       return "AUDIO_EVENT_VOLUME_DOWN";
    case AUDIO_EVENT_SOFTWARE_ISR_OCCURRED: return "AUDIO_EVENT_SOFTWARE_ISR_OCCURRED";

    default:
      break;
  }
  return "AUDIO_EVENT UNKNOWN";
}

/*****************************************************************************/
// Thread-safe functions

void audio_power_on()
{
  audio_event_t event = {.type = AUDIO_EVENT_POWER_ON };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_power_off()
{
  audio_event_t event = {.type = AUDIO_EVENT_POWER_OFF };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}


void audio_stop()
{
  audio_event_t event = {.type = AUDIO_EVENT_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_pause()
{
  audio_event_t event = {.type = AUDIO_EVENT_PAUSE };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_unpause()
{
  audio_event_t event = {.type = AUDIO_EVENT_UNPAUSE };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

/*
 * Handles AUDIO_EVENT_SET_MUTE events.
 * Mutes and Unmutes the audio.
 * If "mute" is true, the audio is muted.
 * If "mute" is false, the audio is unmuted.
 */
static void audio_set_mute_int(bool mute)
{
  // Volume and Mute Control Register (RegAddr 7):
  // Write master mute.
  ag_context.mute = mute;

  if (AUDIO_STATE_OFF != ag_context.state) {
    SSM2518_WriteReg(&I2C4_RTOS_HANDLE, SSM2518_Volume_Mute_Control, ag_context.mute);
  }
}

void audio_set_mute(bool mute)
{
  bool* data = AUDIO_MALLOC(bool);
  if(data!=NULL){
    *data = mute;
    audio_event_t event = { .type = AUDIO_EVENT_SET_MUTE, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

bool audio_get_mute()
{
  return ag_context.mute;
}

/*
 * Handles AUDIO_EVENT_SET_VOLUME events.
 * Sets the volume.
 * volume can range from 0 to 255, 0 = mute, 255 = max vol.
 * Set 'update_ble' when the volume change event DID NOT originate from BLE.
 */
static void audio_set_volume_int(uint8_t volume, bool update_ble)
{
  ag_context.log_volume = volume;
  ag_context.lin_volume = log2lin[volume];

  if(AUDIO_STATE_OFF != ag_context.state) {
    // Invert the volume logic.
    // The SSM2518 considers smaller values = larger volume.
    // This ensures larger values = larger volume
    uint8_t vol_linear_uint8_inv = 255 - ag_context.lin_volume;
    // Left Channel Volume Control Register (RegAddr 5):
    SSM2518_WriteReg(&I2C4_RTOS_HANDLE, SSM2518_Left_Volume_Control, vol_linear_uint8_inv);
    // Right Channel Volume Control Register (RegAddr 6):
    SSM2518_WriteReg(&I2C4_RTOS_HANDLE, SSM2518_Right_Volume_Control, vol_linear_uint8_inv);
  }

  // Tell BLE that volume has changed
  if(update_ble){
    ble_volume_update(volume);
  }
}

void audio_set_volume(uint8_t log_volume)
{
  uint8_t* data = AUDIO_MALLOC(uint8_t);
  if(data!=NULL){
    *data = log_volume;
    audio_event_t event = {.type = AUDIO_EVENT_SET_VOLUME, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_set_volume_ble(uint8_t log_volume)
{
  uint8_t* data = AUDIO_MALLOC(uint8_t);
  if(data!=NULL){
    *data = log_volume;
    audio_event_t event = {.type = AUDIO_EVENT_SET_VOLUME_BLE, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_get_volume(uint8_t* log_volume, uint8_t* lin_volume)
{
  // Return the volume
  *lin_volume = ag_context.lin_volume;
  *log_volume = ag_context.log_volume;
}

/*
 * Handles AUDIO_EVENT_SET_VOLUME_STEP events.
 * Sets the step-size the volume changes for VOLUME_UP and VOLUME_DOWN events.
 */
static void audio_set_volume_step_int(uint8_t step)
{
  ag_context.log_volume_step = step;
}

void audio_set_volume_step(uint8_t log_volume_step)
{
  uint8_t* data = AUDIO_MALLOC(uint8_t);
  if(data!=NULL){
    *data = log_volume_step;
    audio_event_t event = {.type = AUDIO_EVENT_SET_VOLUME_STEP, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

/*
 * Handles AUDIO_EVENT_VOLUME_UP events.
 * Increases the volume by 'step' size set by function "audio_set_volume_step()".
 * If volume+step > 255, volume = 255.
 */
static void audio_volume_up_int(void)
{
  uint8_t vol = ag_context.log_volume;
  if( vol <= 255 - ag_context.log_volume_step ){
    vol += ag_context.log_volume_step;
  }else{
    vol = 255;
  }
  // set the volume immediately
  audio_set_volume_int(vol, true);
}

void audio_volume_up(void)
{
  audio_event_t event = {.type = AUDIO_EVENT_VOLUME_UP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

/*
 * Handles AUDIO_EVENT_VOLUME_DOWN events.
 * Decreases the volume by 'step' size set by function "audio_set_volume_step()".
 * If volume-step < 0, volume = 0.
 */
static void audio_volume_down_int()
{
  uint8_t vol = ag_context.log_volume;
  if( vol >= ag_context.log_volume_step ){
    vol -= ag_context.log_volume_step;
  }else{
    vol = 0;
  }
  // set the volume immediately
  audio_set_volume_int(vol, true);
}

void audio_volume_down()
{
  audio_event_t event = {.type = AUDIO_EVENT_VOLUME_DOWN };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void audio_power_off_timeout(TimerHandle_t timer_handle){
  audio_event_t event = {.type = AUDIO_EVENT_POWER_OFF_TIMEOUT };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

/*************************************/
// FOREGROUND

void audio_fgwav_play(char* filename, bool loop)
{
  // setup the event data
  if(filename == NULL){
    LOGE(TAG,"Received null filename to play.");
    return;
  }

  play_file_data_t* data = AUDIO_MALLOC(play_file_data_t);
  if(data!=NULL){
    strcpy(data->filename, filename);
    data->loop = loop;
    // create and send event
    audio_event_t event = {.type = AUDIO_EVENT_FGWAV_PLAY, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_fgwav_stop()
{
  audio_event_t event = {.type = AUDIO_EVENT_FGWAV_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_fg_fadein(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_FG_FADE_IN, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_fg_fadeout(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_FG_FADE_OUT, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

/*************************************/
// BACKGROUND

void audio_bgwav_play(char* filename, bool loop)
{
  // setup the event data
  if(filename == NULL){
    LOGE(TAG,"Received null filename to play.");
    return;
  }

  play_file_data_t* data = AUDIO_MALLOC(play_file_data_t);
  if(data!=NULL){
    strcpy(data->filename, filename);
    data->loop = loop;
    audio_event_t event = {.type = AUDIO_EVENT_BGWAV_PLAY, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_bgwav_stop()
{
  audio_event_t event = {.type = AUDIO_EVENT_BGWAV_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_bg_fadein(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_BG_FADE_IN, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_bg_fadeout(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_BG_FADE_OUT, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

static inline void audio_bg_set_mixer_gain(float gain_script, float gain_computed, audio_mute_t mute){
#if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
  // save script gain
  if(gain_script>=0){
    bg_gain_script = gain_script;
  }
  // save internal gain
  if(gain_computed>=0){
    bg_gain_computed = gain_computed;
  }
  // save mute
  switch(mute){
  case MUTE_FALSE:
    bg_mute = false;
    break;
  case MUTE_TRUE:
    bg_mute = true;
    break;
  case MUTE_KEEP:
    // do nothing
    break;
  }
  // compute overall gain
  float gain = bg_mute ? 0 : bg_gain_script*bg_gain_computed;
  // TODO: implement a threadsafe way of protecting these without a full
  //       critical section.
//  taskENTER_CRITICAL();
  mixerLeft.gain(AUDIO_BG_WAV_CHANNEL,gain);
  mixerRight.gain(AUDIO_BG_WAV_CHANNEL,gain);
//  taskEXIT_CRITICAL();
#endif
}

void audio_bg_script_volume(float gain){
  audio_bg_set_mixer_gain( gain, GAIN_KEEP, MUTE_KEEP);
}

void audio_bg_computed_volume(float gain){
  audio_bg_set_mixer_gain( GAIN_KEEP, gain, MUTE_KEEP);
}

void audio_bg_default_volume(){
  audio_bg_set_mixer_gain( 0, 1, MUTE_FALSE);
}

/*************************************/
// MP3

void audio_mp3_play(const char* filename, bool loop) {
  if(filename == NULL){
    LOGE(TAG,"Received null filename to play.");
    return;
  }
  
  play_file_data_t* data = AUDIO_MALLOC(play_file_data_t);
  if(data!=NULL){
    strcpy(data->filename, filename);
    data->loop = loop;
    // create and send event
    audio_event_t event = {.type = AUDIO_EVENT_MP3_PLAY, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_mp3_stop() {
  audio_event_t event = {.type = AUDIO_EVENT_MP3_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_mp3_fadein(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_MP3_FADE_IN, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_mp3_fadeout(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_MP3_FADE_OUT, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

/*************************************/
// PINK

void audio_pink_play()
{
  audio_event_t event = {.type = AUDIO_EVENT_PINK_PLAY };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_pink_stop()
{
  audio_event_t event = {.type = AUDIO_EVENT_PINK_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_pink_fadein(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_PINK_FADE_IN, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

void audio_pink_fadeout(uint32_t dur_ms)
{
  uint32_t* data = AUDIO_MALLOC(uint32_t);
  if(data!=NULL){
    *data = dur_ms;
    audio_event_t event = {.type = AUDIO_EVENT_PINK_FADE_OUT, .user_data = data };
    xQueueSend(g_event_queue, &event, portMAX_DELAY);
  }
}

static inline void audio_pink_set_mixer_gain(float gain_script, float gain_computed, audio_mute_t mute){
  // save script gain
  if(gain_script>=0){
    pink_gain_script = gain_script;
  }
  // save internal gain
  if(gain_computed>=0){
    pink_gain_computed = gain_computed;
  }
  // save mute
  switch(mute){
  case MUTE_FALSE:
    pink_mute = false;
    break;
  case MUTE_TRUE:
    pink_mute = true;
    break;
  case MUTE_KEEP:
    // do nothing
    break;
  }
  // compute overall gain
  float gain = pink_mute ? 0 : pink_gain_script*pink_gain_computed;
  // TODO: implement a threadsafe way of protecting these without a full
  //       critical section.
//  taskENTER_CRITICAL();
  mixerLeft.gain(AUDIO_PINK_CHANNEL,gain);
  mixerRight.gain(AUDIO_PINK_CHANNEL,gain);
//  taskEXIT_CRITICAL();
}

void audio_pink_script_volume(float gain){
  audio_pink_set_mixer_gain( gain, GAIN_KEEP, MUTE_KEEP );
}

void audio_pink_computed_volume(float gain){
  audio_pink_set_mixer_gain( GAIN_KEEP, gain, MUTE_KEEP );
}

void audio_pink_mute(bool mute){
  audio_pink_set_mixer_gain( GAIN_KEEP, GAIN_KEEP, mute ? MUTE_TRUE : MUTE_FALSE );
}

void audio_pink_default_volume(){
  audio_pink_set_mixer_gain( 0, 1, MUTE_TRUE);
}

/*************************************/
// SINE

void audio_sine_play()
{
  audio_event_t event = {.type = AUDIO_EVENT_SINE_PLAY };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void audio_sine_stop()
{
  audio_event_t event = {.type = AUDIO_EVENT_SINE_STOP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

/*****************************************************************************/
// State machine

static void
log_event(audio_event_t *event)
{
  switch (event->type) {
  case AUDIO_EVENT_SOFTWARE_ISR_OCCURRED:
    // squelch this one to avoid spamming the console
    break;

  case AUDIO_EVENT_SET_MUTE:
      LOGV(TAG, "[%s] Event: %s %d",
          audio_state_name(ag_context.state),
          audio_event_type_name(event->type),
          *(bool*)(event->user_data));
      break;

    case AUDIO_EVENT_SET_VOLUME:
      LOGV(TAG, "[%s] Event: %s %d",
          audio_state_name(ag_context.state),
          audio_event_type_name(event->type),
          *(uint8_t*)(event->user_data));
      break;

    case AUDIO_EVENT_SET_VOLUME_BLE:
      LOGV(TAG, "[%s] Event: %s %d",
          audio_state_name(ag_context.state),
          audio_event_type_name(event->type),
          *(uint8_t*)(event->user_data));
      break;

    case AUDIO_EVENT_SET_VOLUME_STEP:
      LOGV(TAG, "[%s] Event: %s %d",
          audio_state_name(ag_context.state),
          audio_event_type_name(event->type),
          *(uint8_t*)(event->user_data));
      break;

    case AUDIO_EVENT_VOLUME_UP:
      LOGV(TAG, "[%s] Event: %s",
          audio_state_name(ag_context.state),
          audio_event_type_name(event->type));
      break;

    case AUDIO_EVENT_VOLUME_DOWN:
      LOGV(TAG, "[%s] Event: %s",
          audio_state_name(ag_context.state),
          audio_event_type_name(event->type));
      break;

    default:
      LOGV(TAG, "[%s] Event: %s",
          audio_state_name(ag_context.state),
          audio_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(audio_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s",
      audio_state_name(ag_context.state),
      audio_event_type_name(event->type));
}

static void
set_state(audio_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]",
      audio_state_name(ag_context.state),
      audio_state_name(state));

  ag_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  // Warning! Be careful not to allow this function to be called from other 
  // tasks. And/or ensure the handling of this event is thread-safe.
  audio_state_t* data = AUDIO_MALLOC(audio_state_t);
  *data = state;
  audio_event_t event = { .type=AUDIO_EVENT_ENTER_STATE, .user_data = data };
  handle_event(&event);
}

//
// Event handlers for the various application states:
//

static void
handle_state_off(audio_event_t *event)
{
  switch (event->type) {
    case AUDIO_EVENT_ENTER_STATE:
      // Turn off power off timer
      xTimerStop(ag_context.audio_power_off_timer_handle, portMAX_DELAY);

      AudioOutputI2S::end();
      // pull shutdown pin low - power down
      GPIO_PinWrite(
          BOARD_INITPINS_SSM2518_SHTDNn_GPIO,
          BOARD_INITPINS_SSM2518_SHTDNn_PORT,
          BOARD_INITPINS_SSM2518_SHTDNn_PIN,
          0);
      break;

    case AUDIO_EVENT_POWER_ON:
      // pull shutdown pin high - power up
      GPIO_PinWrite(
          BOARD_INITPINS_SSM2518_SHTDNn_GPIO,
          BOARD_INITPINS_SSM2518_SHTDNn_PORT,
          BOARD_INITPINS_SSM2518_SHTDNn_PIN,
          1);
      vTaskDelay(pdMS_TO_TICKS(10));
      // initialize the SSM2518
      SSM2518_Init(&I2C4_RTOS_HANDLE);
      set_state(AUDIO_STATE_STANDBY);

      // Restore settings
      audio_set_mute_int(ag_context.mute);
      audio_set_volume_int(ag_context.log_volume, false);

      // Enable audio
      AudioOutputI2S::begin();

      break;

    // Any of the following events will wake the audio
    case AUDIO_EVENT_FGWAV_PLAY:    // fall through
    case AUDIO_EVENT_BGWAV_PLAY:    // fall through
    case AUDIO_EVENT_SINE_PLAY:     // fall through
    case AUDIO_EVENT_MP3_PLAY:      // fall through
    case AUDIO_EVENT_PINK_PLAY:     // fall through
    case AUDIO_EVENT_FG_FADE_IN:    // fall through
    case AUDIO_EVENT_FG_FADE_OUT:   // fall through
    case AUDIO_EVENT_BG_FADE_IN:    // fall through
    case AUDIO_EVENT_BG_FADE_OUT:   // fall through
    case AUDIO_EVENT_MP3_FADE_IN:   // fall through
    case AUDIO_EVENT_MP3_FADE_OUT:  // fall through
    case AUDIO_EVENT_PINK_FADE_IN:  // fall through
    case AUDIO_EVENT_PINK_FADE_OUT: // fall through
    {
      // Handle a power on event
      static audio_event_t event_power_on = {.type = AUDIO_EVENT_POWER_ON };
      log_event(&event_power_on);
      handle_event(&event_power_on);
      // Forward the current event
      log_event(event);
      handle_event(event);

//      // Enqueue a power on event
//      static const audio_event_t event_power_on = {.type = AUDIO_EVENT_POWER_ON };
//      xQueueSend(g_event_queue, &event_power_on, portMAX_DELAY);
//      // Then forward the current event
//      xQueueSend(g_event_queue, event, portMAX_DELAY);
      break;
    }

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_standby(audio_event_t *event)
{
  switch (event->type) {
    case AUDIO_EVENT_ENTER_STATE:
      // Warning: this can be invoked from other tasks!
      break;

    case AUDIO_EVENT_POWER_OFF:
      set_state(AUDIO_STATE_OFF);
      break;

#if (defined(AUDIO_ENABLE_FG_WAV) && (AUDIO_ENABLE_FG_WAV > 0U))
    case AUDIO_EVENT_FGWAV_PLAY:
    {
      mixerLeft.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain=1);
      mixerRight.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain=1);
      play_file_data_t* play_file_data = event->data.play_file_data;
      char* filename = play_file_data->filename;
      bool loop = play_file_data->loop;
      bool success = wavFG.play(filename, loop);
      if (success) {
        LOGV(TAG,"Playing file: %s", filename);
      }else{
        // Failed to open file
        LOGE(TAG,"Failed to play file: %s", filename);
      }
      break;
    }

    case AUDIO_EVENT_FGWAV_STOP:
      mixerLeft.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain=0);
      mixerRight.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain=0);
      wavFG.stop();
      break;

    case AUDIO_EVENT_FG_FADE_IN:
    {
      play_file_data_t* event_data = (audio_event_fade_data_t*) event->data;
      fade_fg_left.fadeIn( event_data->dur_ms );
      fade_fg_right.fadeIn( event_data->dur_ms );
      break;
    }

    case AUDIO_EVENT_FG_FADE_OUT:
    {
      play_file_data_t* event_data = (audio_event_fade_data_t*) event->data;
      fade_fg_left.fadeOut( event_data->dur_ms );
      fade_fg_right.fadeOut( event_data->dur_ms );
      break;
    }
#endif

#if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
    case AUDIO_EVENT_BGWAV_PLAY:
    {
//      // TODO: move volume adjustment outside
//      mixerLeft.gain(AUDIO_BG_WAV_CHANNEL,gain_bgwav=0.2);
//      mixerRight.gain(AUDIO_BG_WAV_CHANNEL,gain_bgwav=0.2);
      play_file_data_t* play_file_data = (play_file_data_t*)(event->user_data);
      char* filename = play_file_data->filename;
      bool loop = play_file_data->loop;
      bool success = wavBG.play(filename, loop); // looping
      if (success) {
        LOGV(TAG,"Playing file: %s", filename);
      }else{
        // Failed to open file
        LOGE(TAG,"Failed to play file: %s", filename);
      }
      break;
    }

    case AUDIO_EVENT_BGWAV_STOP:
//      mixerLeft.gain(AUDIO_BG_WAV_CHANNEL,gain_bgwav=0);
//      mixerRight.gain(AUDIO_BG_WAV_CHANNEL,gain_bgwav=0);
      wavBG.stop();
      LOGV(TAG,"audio mem usg max: %u", (unsigned int) AudioMemoryUsageMax());
      break;

    case AUDIO_EVENT_BG_FADE_IN:
    {
      bg_fade_left.fadeIn( *(uint32_t*)(event->user_data) );
      bg_fade_right.fadeIn( *(uint32_t*)(event->user_data) );
      break;
    }

    case AUDIO_EVENT_BG_FADE_OUT:
    {
      bg_fade_left.fadeOut( *(uint32_t*)(event->user_data) );
      bg_fade_right.fadeOut( *(uint32_t*)(event->user_data) );
      break;
    }
#endif

#if (defined(AUDIO_ENABLE_MP3) && (AUDIO_ENABLE_MP3 > 0U))
  case AUDIO_EVENT_MP3_PLAY:
  {
    play_file_data_t* play_file_data = (play_file_data_t*)(event->user_data);
    mp3output.open(play_file_data->filename);
    mp3output.set_loop(play_file_data->loop);
    mp3output.play();
    mixerLeft.gain(AUDIO_MP3_CHANNEL,mp3_gain=1);
    mixerRight.gain(AUDIO_MP3_CHANNEL,mp3_gain=1);
    break;
  }
  case AUDIO_EVENT_MP3_STOP:
    mp3output.stop();
    mixerLeft.gain(AUDIO_MP3_CHANNEL,mp3_gain=0);
    mixerRight.gain(AUDIO_MP3_CHANNEL,mp3_gain=0);
    break;

  case AUDIO_EVENT_MP3_FADE_IN:
  {
    fade_mp3_left.fadeIn( *(uint32_t*)(event->user_data) );
    fade_mp3_right.fadeIn( *(uint32_t*)(event->user_data) );
    break;
  }

  case AUDIO_EVENT_MP3_FADE_OUT:
  {
    fade_mp3_left.fadeOut( *(uint32_t*)(event->user_data) );
    fade_mp3_right.fadeOut( *(uint32_t*)(event->user_data) );
    break;
  }
#endif

#if (defined(AUDIO_ENABLE_SINE) && (AUDIO_ENABLE_SINE > 0U))
    case AUDIO_EVENT_SINE_PLAY:
      sineLow.amplitude(1);
      sineHigh.amplitude(1);
      mixerLeft.gain(AUDIO_SINE_CHANNEL,sine_gain=1);
      mixerRight.gain(AUDIO_SINE_CHANNEL,sine_gain=1);
      break;

    case AUDIO_EVENT_SINE_STOP:
      sineLow.amplitude(0);
      sineHigh.amplitude(0);
      mixerLeft.gain(AUDIO_SINE_CHANNEL,sine_gain=0);
      mixerRight.gain(AUDIO_SINE_CHANNEL,sine_gain=0);
      break;
#endif

#if (defined(AUDIO_ENABLE_PINK) && (AUDIO_ENABLE_PINK > 0U))
    case AUDIO_EVENT_PINK_PLAY:
      pink.amplitude(1);
//      mixerLeft.gain(AUDIO_PINK_CHANNEL,gain_pink=1);
//      mixerRight.gain(AUDIO_PINK_CHANNEL,gain_pink=1);
      break;

  case AUDIO_EVENT_PINK_STOP:
      pink.amplitude(0);
//      mixerLeft.gain(AUDIO_PINK_CHANNEL,gain_pink=0);
//      mixerRight.gain(AUDIO_PINK_CHANNEL,gain_pink=0);
      break;

    case AUDIO_EVENT_PINK_FADE_IN:
    {
      pink_fade.fadeIn( *(uint32_t*)(event->user_data) );
      break;
    }

    case AUDIO_EVENT_PINK_FADE_OUT:
    {
      pink_fade.fadeOut( *(uint32_t*)(event->user_data) );
      break;
    }
#endif

    case AUDIO_EVENT_STOP:
      // TODO: Stop all audio playback and fade out immediately
      break;

    case AUDIO_EVENT_PAUSE:
#if (defined(AUDIO_ENABLE_FG_WAV) && (AUDIO_ENABLE_FG_WAV > 0U))
      mixerLeft.gain(AUDIO_FG_WAV_CHANNEL,0);
      mixerRight.gain(AUDIO_FG_WAV_CHANNEL,0);
#endif
#if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
      audio_bg_set_mixer_gain( GAIN_KEEP, GAIN_KEEP, MUTE_TRUE);
#endif
#if (defined(AUDIO_ENABLE_MP3) && (AUDIO_ENABLE_MP3 > 0U))
      mixerLeft.gain(AUDIO_MP3_CHANNEL,0);
      mixerRight.gain(AUDIO_MP3_CHANNEL,0);
#endif
#if (defined(AUDIO_ENABLE_PINK) && (AUDIO_ENABLE_PINK > 0U))
      audio_pink_set_mixer_gain( GAIN_KEEP, GAIN_KEEP, MUTE_TRUE);
#endif
#if (defined(AUDIO_ENABLE_SINE) && (AUDIO_ENABLE_SINE > 0U))
      mixerLeft.gain(AUDIO_SINE_CHANNEL,0);
      mixerRight.gain(AUDIO_SINE_CHANNEL,0);
#endif
      break;

    case AUDIO_EVENT_UNPAUSE:
#if (defined(AUDIO_ENABLE_FG_WAV) && (AUDIO_ENABLE_FG_WAV > 0U))
      mixerLeft.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain);
      mixerRight.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain);
#endif
#if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
      audio_bg_set_mixer_gain( GAIN_KEEP, GAIN_KEEP, MUTE_FALSE);
#endif
#if (defined(AUDIO_ENABLE_MP3) && (AUDIO_ENABLE_MP3 > 0U))
      mixerLeft.gain(AUDIO_MP3_CHANNEL,mp3_gain);
      mixerRight.gain(AUDIO_MP3_CHANNEL,mp3_gain);
#endif
#if (defined(AUDIO_ENABLE_PINK) && (AUDIO_ENABLE_PINK > 0U))
      audio_pink_set_mixer_gain( GAIN_KEEP, GAIN_KEEP, MUTE_FALSE);
#endif
#if (defined(AUDIO_ENABLE_SINE) && (AUDIO_ENABLE_SINE > 0U))
      mixerLeft.gain(AUDIO_SINE_CHANNEL,sine_gain);
      mixerRight.gain(AUDIO_SINE_CHANNEL,sine_gain);
#endif
      break;

    case AUDIO_EVENT_SOFTWARE_ISR_OCCURRED:
      // Handle the software interrupt, but in a scheduled, non-ISR context:
      AudioOutputI2S::handle_audio_i2s_event();

      static bool prev_audio_idle = false, curr_audio_idle = false;
      prev_audio_idle = curr_audio_idle;

      if (
        1 // this helps enable the compile time flags below ...
        #if (defined(AUDIO_ENABLE_FG_WAV) && (AUDIO_ENABLE_FG_WAV > 0U))
        && wavFG.is_idle()
        && fade_fg_left.is_idle()
        && fade_fg_right.is_idle()
        #endif
        #if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
        && wavBG.is_idle()
        && bg_fade_left.is_idle()
        && bg_fade_right.is_idle()
        #endif
        #if (defined(AUDIO_ENABLE_PINK) && (AUDIO_ENABLE_PINK > 0U))
        && pink.is_idle()
        #endif
        #if (defined(AUDIO_ENABLE_SINE) && (AUDIO_ENABLE_SINE > 0U))
        && sineLow.is_idle()
        && sineHigh.is_idle()
        #endif
        #if (defined(AUDIO_ENABLE_MP3) && (AUDIO_ENABLE_MP3 > 0U))
        && mp3output.is_idle()
        #endif
        ) {
        curr_audio_idle = true;
      }else{
        curr_audio_idle = false;
      }

      // We need to use a delay-timer strategy to turn off power,
      // because wavBG playback starts by sending an event to
      // wav_buffer's message queue to tell wav_buffer to start running.
      // That message can take a little while to process before wav_buffer actually starts running.
      // But the way we check whether the wavBG playback is idle only considers whether wav_buffer task is running.
      if (      !prev_audio_idle &&  curr_audio_idle) {
        xTimerStart(ag_context.audio_power_off_timer_handle, portMAX_DELAY);
      } else if( prev_audio_idle && !curr_audio_idle) {
        xTimerStop(ag_context.audio_power_off_timer_handle, portMAX_DELAY);
      }
      break;

    case AUDIO_EVENT_POWER_OFF_TIMEOUT:
      set_state(AUDIO_STATE_OFF);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_event(audio_event_t *event)
{
  // handle non-state dependent events
  switch (event->type) {
    case AUDIO_EVENT_SET_MUTE:
      audio_set_mute_int(*(bool*)(event->user_data));
      return;

    case AUDIO_EVENT_SET_VOLUME:
      audio_set_volume_int(*(uint8_t*)(event->user_data), true);
      return;

    case AUDIO_EVENT_SET_VOLUME_BLE:
      audio_set_volume_int(*(uint8_t*)(event->user_data), false);
      return;

    case AUDIO_EVENT_SET_VOLUME_STEP:
      audio_set_volume_step_int(*(uint8_t*)(event->user_data));
      return;

    case AUDIO_EVENT_VOLUME_UP:
      audio_volume_up_int();
      return;

    case AUDIO_EVENT_VOLUME_DOWN:
      audio_volume_down_int();
      return;

    default:
      break;
  }
  // handle state dependent events
  switch (ag_context.state) {
    case AUDIO_STATE_OFF:
      handle_state_off(event);
      break;

    case AUDIO_STATE_STANDBY:
      handle_state_standby(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown audio state: %d", (int) ag_context.state);
      break;
  }
}

void
audio_pretask_init(void)
{
  // Any pre-scheduler init goes here.
  // setup shutdown pin
  gpio_pin_config_t SSM2518_SHTDNn_config = {
      .pinDirection = kGPIO_DigitalOutput,
      .outputLogic = 0U
  };
  GPIO_PinInit(
      BOARD_INITPINS_SSM2518_SHTDNn_GPIO,
      BOARD_INITPINS_SSM2518_SHTDNn_PORT,
      BOARD_INITPINS_SSM2518_SHTDNn_PIN,
      &SSM2518_SHTDNn_config);

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(AUDIO_EVENT_QUEUE_SIZE,sizeof(audio_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "audio_event_queue");

  // Create the event memory
  mm_rtos_init( &g_event_memory, g_event_memory_buf, sizeof(g_event_memory_buf) );

  ag_context.audio_power_off_timer_handle = xTimerCreateStatic("AUDIO_POWER_OFF_TIMER",
    pdMS_TO_TICKS(AUDIO_IDLE_2_POWER_OFF_DELAY_MS), pdFALSE, NULL,
    audio_power_off_timeout, &(ag_context.audio_power_off_timer_struct));

  //// Setup the PJRC Audio processing pipeline ////

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioStream::initialize_memory(AUDIO_DATA, AUDIO_DATA_SIZE);


#if (defined(AUDIO_ENABLE_FG_WAV) && (AUDIO_ENABLE_FG_WAV > 0U))
  wavFG.pretask_init();
#endif
#if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
  wavBG.pretask_init();
#endif

  // TODO: Set gain back to 0.
#if (defined(AUDIO_ENABLE_FG_WAV) && (AUDIO_ENABLE_FG_WAV > 0U))
  mixerLeft.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain=0);
  mixerRight.gain(AUDIO_FG_WAV_CHANNEL,fgwav_gain=0);
#endif
#if (defined(AUDIO_ENABLE_BG_WAV) && (AUDIO_ENABLE_BG_WAV > 0U))
  audio_bg_default_volume();
#endif
#if (defined(AUDIO_ENABLE_MP3) && (AUDIO_ENABLE_MP3 > 0U))
  mixerLeft.gain(AUDIO_MP3_CHANNEL,mp3_gain=0);
  mixerRight.gain(AUDIO_MP3_CHANNEL,mp3_gain=0);
#endif
#if (defined(AUDIO_ENABLE_PINK) && (AUDIO_ENABLE_PINK > 0U))
  audio_pink_default_volume();
#endif
#if (defined(AUDIO_ENABLE_SINE) && (AUDIO_ENABLE_SINE > 0U))
  mixerLeft.gain(AUDIO_SINE_CHANNEL,sine_gain=0);
  mixerRight.gain(AUDIO_SINE_CHANNEL,sine_gain=0);
#endif

#if (defined(AUDIO_ENABLE_SINE) && (AUDIO_ENABLE_SINE > 0U))
  sineLow.amplitude(0);
  sineLow.frequency(440);
  sineHigh.amplitude(0);
  sineHigh.frequency(880);
#endif

#if (defined(AUDIO_ENABLE_PINK) && (AUDIO_ENABLE_PINK > 0U))
  pink.amplitude(0);
#endif

}


static void
task_init()
{
  // Any post-scheduler init goes here.

  // Initialize the streaming block
  AudioOutputI2S::init();

  // Start off with the audio driver disabled.
  // Note this is not a deferred call.
  set_state(AUDIO_STATE_OFF);

  // Set the default settings
  ag_context.mute = false;
  ag_context.log_volume_step = AUDIO_VOLUME_STEP;
  ag_context.log_volume = 100; // TODO: This causes BLE task to crash

  // This is a deferred call which enables the audio
//  audio_power_on();

  LOGV(TAG, "Task launched. Entering event loop.");
}


void
audio_task(void *ignored)
{

  task_init();

  audio_event_t event;

  while (1) {
    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

    // free the malloc'd memory
    if( event.user_data!= NULL){
      mm_rtos_free(&g_event_memory, event.user_data);
    }
  }
}



/*
 * Pin change interrupt callback for audio update ISR,
 * This interrupt is NOT routed to a pin, but is intended to be software triggered.
 */
#if defined(VARIANT_NFF1) || defined(VARIANT_FF1) || defined(VARIANT_FF2) || defined(VARIANT_FF3) || defined(VARIANT_FF4)

#define HALT_IF_DEBUGGING()                              \
  do {                                                   \
    if ((*(volatile uint32_t *)0xE000EDF0) & (1 << 0)) { \
      __asm("bkpt 1");                                   \
    }                                                    \
  } while (0)

// Non-ISR version of this function
void audio_event_update_streams(void)
{
  // NOTE: This is called immediately upon boot-up due to I2S DMA ISR, before the scheduler
  // is started. So we guard against that (we can't call xQueueSend() without a scheduler):
  audio_event_t event = {.type = AUDIO_EVENT_SOFTWARE_ISR_OCCURRED };
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    xQueueSend(g_event_queue, &event, 0);
  }
}

//void audio_software_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
void audio_event_update_streams_from_isr(status_t i2s_completion_status)
{
  TRACEALYZER_ISR_AUDIO_BEGIN( AUDIO_I2S_ISR_TRACE );

  if (i2s_completion_status == kStatus_I2S_BufferComplete) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    audio_event_t event = {.type = AUDIO_EVENT_SOFTWARE_ISR_OCCURRED };
    xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);

    // Always do this when calling a FreeRTOS "...FromISR()" function:
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }else{
    HALT_IF_DEBUGGING();
  }

  TRACEALYZER_ISR_AUDIO_END( xHigherPriorityTaskWoken );
}

#endif

#else // #if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))

void audio_pretask_init(void){}
void audio_task(void *ignored){}

void audio_power_on(){}
void audio_power_off(){}
void audio_stop(){}
void audio_pause(){}
void audio_unpause(){}
void audio_set_mute(bool m){}
bool audio_get_mute(){return false;}
void audio_set_volume(uint8_t vol){}
void audio_set_volume_ble(uint8_t vol){}
uint8_t audio_get_volume(){return 0;}
void audio_set_volume_step(uint8_t step){}
void audio_volume_up(){}
void audio_volume_down(){}

void audio_fgwav_play(char* filename, bool loop){}
void audio_fgwav_stop(){}
void audio_fg_fadein(uint32_t dur_ms){}
void audio_fg_fadeout(uint32_t dur_ms){}

void audio_bgwav_play(char* filename, bool loop){}
void audio_bgwav_stop(){}
void audio_bg_fadein(uint32_t dur_ms){}
void audio_bg_fadeout(uint32_t dur_ms){}
void audio_bg_script_volume(float gain){}
void audio_bg_computed_volume(float gain){}

void audio_mp3_play(const char* filename, bool loop){}
void audio_mp3_stop(){}

void audio_pink_play(){}
void audio_pink_stop(){}
void audio_pink_fadein(uint32_t dur_ms){}
void audio_pink_fadeout(uint32_t dur_ms){}
void audio_pink_script_volume(float gain){}
void audio_pink_computed_volume(float gain){}
void audio_pink_mute(bool mute){}

void audio_sine_play(){}
void audio_sine_stop(){}

void audio_event_update_streams(void){}
void audio_event_update_streams_from_isr(void){}


#endif // #if (defined(ENABLE_AUDIO_TASK) && (ENABLE_AUDIO_TASK > 0U))
