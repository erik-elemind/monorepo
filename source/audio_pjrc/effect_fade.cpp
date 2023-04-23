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

//#include <Arduino.h>
#include "effect_fade.h"
#include "utility/dspinst.h"
#include "utils.h"

/*****************************************************************************/
// class AudioEffectFade

extern "C" {
extern const int16_t fader_table[257];
};

void AudioEffectFade::update(void)
{
	audio_block_t *block;
	uint32_t i, pos, inc, index, scale;
	int32_t val1, val2, val, sample;
	uint8_t dir;

	pos = position;

	if (pos == 0) {
		// output is silent
		block = receiveReadOnly();
		if (block) release(block);
		return;
	} else if (pos == 0xFFFFFFFF) {
		// output is 100%
		block = receiveReadOnly();
		if (!block) return;
		transmit(block);
		release(block);
		return;
	}
	block = receiveWritable();
	if (!block) return;
	inc = rate;
	dir = direction;
	for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		index = pos >> 24;
		val1 = fader_table[index];
		val2 = fader_table[index+1];
		scale = (pos >> 8) & 0xFFFF;
		val2 *= scale;
		val1 *= 0x10000 - scale;
		val = (val1 + val2) >> 16;
		sample = block->data[i];
		sample = (sample * val) >> 15;
		block->data[i] = sample;
		if (dir > 0) {
			// output is increasing
			if (inc < 0xFFFFFFFF - pos) pos += inc;
			else pos = 0xFFFFFFFF;
		} else {
			// output is decreasing
			if (inc < pos) pos -= inc;
			else pos = 0;
		}
	}

	position = pos;
	transmit(block);
	release(block);
}

bool AudioEffectFade::is_idle(void)
{
  AUDIO_ENTER_CRITICAL();
  uint32_t pos = position;
  AUDIO_EXIT_CRITICAL();
  return pos == 0 || pos == 0xFFFFFFFF;
}

void AudioEffectFade::fadeIn(uint32_t milliseconds) {
  if(milliseconds == 0){
    setVolume(true);
  }else{
    // TODO: change 441u to 500u, to match sampling rate
//          uint32_t samples = (uint32_t)(milliseconds * 441u + 5u) / 10u;
    // changed to 22kHz sampling rate
    uint32_t samples = (uint32_t)(milliseconds * 220u + 5u) / 10u;
      //Serial.printf("fadeIn, %u samples\n", samples);
      fadeBegin(0xFFFFFFFFu / samples, 1);
  }
}

void AudioEffectFade::fadeOut(uint32_t milliseconds) {
    if(milliseconds == 0){
      setVolume(false);
    }else{
    // TODO: change 441u to 500u, to match sampling rate
//          uint32_t samples = (uint32_t)(milliseconds * 441u + 5u) / 10u;
      // changed to 22kHz sampling rate
      uint32_t samples = (uint32_t)(milliseconds * 220u + 5u) / 10u;
      //Serial.printf("fadeOut, %u samples\n", samples);
      fadeBegin(0xFFFFFFFFu / samples, 0);
    }
}

void AudioEffectFade::setVolume(bool vol){
    AUDIO_ENTER_CRITICAL();
    // 0 = silent, 0xFFFFFFFF = pass-through
    position = vol ? 0xFFFFFFFF : 0;
    AUDIO_EXIT_CRITICAL();
}

void AudioEffectFade::fadeBegin(uint32_t newrate, uint8_t dir)
{
    AUDIO_ENTER_CRITICAL();
	uint32_t pos = position;
	if (pos == 0) position = 1;
	else if (pos == 0xFFFFFFFF) position = 0xFFFFFFFE;
	rate = newrate;
	direction = dir;
	AUDIO_EXIT_CRITICAL();
}



