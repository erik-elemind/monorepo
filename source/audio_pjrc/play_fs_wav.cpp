/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <play_fs_wav.h>
#include <play_fs_wav_parser.h>
#include "utils.h"
#include "loglevels.h"
#include "critical_section.h"

/*****************************************************************************/
// class AudioPlayFsWav

//static const char *TAG = "play_fs_wav";   // Logging prefix for this module

void AudioPlayFsWav::begin(void)
{
    if (block_left) {
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		release(block_right);
		block_right = NULL;
	}
}

bool AudioPlayFsWav::play(const char *filename, bool loop)
{
	return start_buffer(filename, loop);
}

void AudioPlayFsWav::stop(void)
{
  AUDIO_ENTER_CRITICAL();
  audio_block_t *b1 = block_left;
  block_left = NULL;
  audio_block_t *b2 = block_right;
  block_right = NULL;
  AUDIO_EXIT_CRITICAL();

  if (b1) release(b1);
  if (b2) release(b2);
  stop_buffer();
}

void AudioPlayFsWav::update_16bit_22_mono(void)
{
  // AUDIO_BLOCK_SAMPLES MUST BE A MULTIPLE OF 2 [bytes]
  size_t exp_buf_len_bytes = 2*AUDIO_BLOCK_SAMPLES;
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
  fs_wav_buffer_return_t buf_result = get_from_buffer(exp_buf_len_bytes);
  int16_t act_buf_len_bytes = buf_result.size;
  buffer = buf_result.data;
#else
  int16_t act_buf_len_bytes = get_from_buffer(&buffer[0], exp_buf_len_bytes);
#endif

  // TODO: ensure the buffer is always filled with paired data for left and right channels.
  if(act_buf_len_bytes % 2 != 0){
    debug_uart_puts((char*)"Odd number of wav bytes:");
    debug_uart_puti(act_buf_len_bytes);
  }

  block_left  = allocate();

#if 0
  if(block_left){
    uint16_t block_offset = 0;
    uint8_t lsb, msb;
    const uint8_t* p = buffer;
    while ( act_buf_len_bytes > 0 ) {
        lsb = *p++;
        msb = *p++;
        act_buf_len_bytes -= 2;
        block_left->data[block_offset++] = (msb << 8) | lsb;
    }

    // If there is not enough data to fill a block, back fill it with 0.
    if( block_offset != AUDIO_BLOCK_SAMPLES ){
      memset(&(block_left->data)[block_offset], 0 , 2*(AUDIO_BLOCK_SAMPLES-block_offset));
    }

    transmit(block_left, 0);
    transmit(block_left, 1);
    release(block_left);
    block_left = NULL;
  }
#endif
  if(block_left){
    memcpy(block_left->data, buffer, act_buf_len_bytes);
    uint16_t block_offset = act_buf_len_bytes/2;

    // If there is not enough data to fill a block, back fill it with 0.
    if( block_offset != AUDIO_BLOCK_SAMPLES ){
      memset(&(block_left->data)[block_offset], 0 , 2*(AUDIO_BLOCK_SAMPLES-block_offset));
    }

    transmit(block_left, 0);
    transmit(block_left, 1);
    release(block_left);
    block_left = NULL;
  }

  release_from_buffer();
}

// TODO: Play back of 16bit, 22kHz stereo is untested.
void AudioPlayFsWav::update_16bit_22_stereo(void)
{
  // AUDIO_BLOCK_SAMPLES MUST BE A MULTIPLE OF 2 [bytes]
  size_t exp_buf_len_bytes = 2*AUDIO_BLOCK_SAMPLES;
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
  fs_wav_buffer_return_t buf_result = get_from_buffer(exp_buf_len_bytes);
  int16_t act_buf_len_bytes = buf_result.size;
  buffer = buf_result.data;
#else
  int16_t act_buf_len_bytes = get_from_buffer(&buffer[0], exp_buf_len_bytes);
#endif

  block_left  = allocate();
  block_right = allocate();

  if(block_left && block_right){
    uint16_t block_offset = 0;
    uint8_t lsb, msb;
    const uint8_t* p = buffer;
    while ( act_buf_len_bytes > 0 ) {
        lsb = *p++;
        msb = *p++;
        act_buf_len_bytes -= 2;
        block_left->data[block_offset] = (msb << 8) | lsb;
        block_left->data[block_offset+1] = (msb << 8) | lsb;

        lsb = *p++;
        msb = *p++;
        act_buf_len_bytes -= 2;
        block_right->data[block_offset] = (msb << 8) | lsb;
        block_right->data[block_offset+1] = (msb << 8) | lsb;
        block_offset+=2;
    }

    // If there is not enough data to fill a block, back fill it with 0.
    if( block_offset != AUDIO_BLOCK_SAMPLES ){
      memset(&(block_left ->data)[block_offset], 0 , 2*(AUDIO_BLOCK_SAMPLES-block_offset));
      memset(&(block_right->data)[block_offset], 0 , 2*(AUDIO_BLOCK_SAMPLES-block_offset));
    }

    transmit(block_left, 0);
    release(block_left);
    block_left = NULL;

    transmit(block_right, 1);
    release(block_right);
    block_right = NULL;
  }else{
    if(block_left){
      release(block_left);
      block_left = NULL;
    }
    if(block_right){
      release(block_right);
      block_right = NULL;
    }
  }

  release_from_buffer();
}


void AudioPlayFsWav::update_16bit_44_mono(void)
{
  // AUDIO_BLOCK_SAMPLES MUST BE A MULTIPLE OF 2 [bytes]
  size_t exp_buf_len_bytes = 2*AUDIO_BLOCK_SAMPLES;
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
  fs_wav_buffer_return_t buf_result = get_from_buffer(exp_buf_len_bytes);
  int16_t act_buf_len_bytes = buf_result.size;
  buffer = buf_result.data;
#else
  int16_t act_buf_len_bytes = get_from_buffer(&buffer[0], exp_buf_len_bytes);
#endif

  block_left  = allocate();

  if(block_left){
    uint16_t block_offset = 0;
    uint8_t lsb, msb;
    const uint8_t* p = buffer;
    while ( act_buf_len_bytes > 0 ) {
        lsb = *p++;
        msb = *p++;
        act_buf_len_bytes -= 2;
        block_left->data[block_offset++] = (msb << 8) | lsb;
    }

    // If there is not enough data to fill a block, back fill it with 0.
    if( block_offset != AUDIO_BLOCK_SAMPLES ){
  //    LOGW(TAG,"no data");
      memset(&(block_left->data)[block_offset], 0 , 2*(AUDIO_BLOCK_SAMPLES-block_offset));
    }

    transmit(block_left, 0);
    transmit(block_left, 1);
    release(block_left);
    block_left = NULL;
  }

  release_from_buffer();
}

void AudioPlayFsWav::update_16bit_44_stereo(void)
{
  // AUDIO_BLOCK_SAMPLES MUST BE A MULTIPLE OF 4 [bytes]
  size_t exp_buf_len_bytes = 4*AUDIO_BLOCK_SAMPLES;
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
  fs_wav_buffer_return_t buf_result = get_from_buffer(exp_buf_len_bytes);
  int16_t act_buf_len_bytes = buf_result.size;
  buffer = buf_result.data;
#else
  int16_t act_buf_len_bytes = get_from_buffer(&buffer[0], exp_buf_len_bytes);
#endif

  block_left  = allocate();
  block_right = allocate();

  if(block_left && block_right){
    uint16_t block_offset = 0;
    uint8_t lsb, msb;
    const uint8_t* p = buffer;
    while ( act_buf_len_bytes > 0 ) {
        lsb = *p++;
        msb = *p++;
        act_buf_len_bytes -= 2;
        block_left->data[block_offset] = (msb << 8) | lsb;

        lsb = *p++;
        msb = *p++;
        act_buf_len_bytes -= 2;
        block_right->data[block_offset++] = (msb << 8) | lsb;
    }

    // If there is not enough data to fill a block, back fill it with 0.
    if( block_offset != AUDIO_BLOCK_SAMPLES ){
      memset(&(block_left->data)[block_offset], 0 , 2*(AUDIO_BLOCK_SAMPLES-block_offset));
      memset(&(block_right->data)[block_offset], 0 , 2*(AUDIO_BLOCK_SAMPLES-block_offset));
    }

    transmit(block_left, 0);
    release(block_left);
    block_left = NULL;

    transmit(block_right, 1);
    release(block_right);
    block_right = NULL;
  }else{
    if(block_left){
      release(block_left);
      block_left = NULL;
    }
    if(block_right){
      release(block_right);
      block_right = NULL;
    }
  }

  release_from_buffer();
}

void AudioPlayFsWav::update(void)
{
// The following chunk of code does NOT work
// It is intended to change how the buffer stream is parsed.

  uint8_t  state_play;
  uint32_t sample_rate;
  // TODO: There is NO reason why "get_stream_params" needs to be called on each update
  // when the values it pulls from parsing the file only change once, when the file
  // is first opened (on a call to play()).
  get_stream_params(state_play, sample_rate);
//  LOGV("play_fs_wav","stream params: %d %lu", state_play, sample_rate);

  switch(state_play){
  case STATE_DIRECT_16BIT_MONO:  // playing mono at native sample rate
    if(sample_rate==44100){
      update_16bit_44_mono();
      return;
    }else if(sample_rate==22050){
      update_16bit_22_mono();
      return;
    }
    break;

  case STATE_DIRECT_16BIT_STEREO:  // playing stereo at native sample rate
    if(sample_rate==44100){
      update_16bit_44_stereo();
      return;
    }else if(sample_rate==22050){
      update_16bit_22_stereo();
      return;
    }
    break;

  default:
    break;
  }

  // send zeroed audio data to left and right channels.
  block_left  = allocate();
  if(block_left){
    memset(&(block_left->data)[0], 0 , 2*AUDIO_BLOCK_SAMPLES);
    transmit(block_left, 0);
    transmit(block_left, 1);
    release(block_left);
    block_left = NULL;
  }

}

bool AudioPlayFsWav::is_idle(void)
{
  return buffer_is_idle();
}


#if 0

bool AudioPlayFsWav::isPlaying(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s < 8);
}

uint32_t AudioPlayFsWav::positionMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t dlength = *(volatile uint32_t *)&data_length;
	uint32_t offset = tlength - dlength;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)offset * b2m) >> 32;
}


uint32_t AudioPlayFsWav::lengthMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)tlength * b2m) >> 32;
}



#endif

