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

#ifndef play_fs_wav_h_
#define play_fs_wav_h_

#include <play_fs_wav_buffer_rtos.h>
#include "AudioStream.h"
#include "config.h"


class AudioPlayFsWav : public AudioStream, public AudioPlayFsWavBufferRTOS
{
public:
	AudioPlayFsWav(void) : AudioStream(0, NULL), block_left(NULL), block_right(NULL) { begin(); }
	virtual void update(void);
	virtual bool is_idle(void);
	bool play(const char *filename, bool loop = false);
	void stop(void);
#if 0
	bool isPlaying(void);
	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
	virtual void update(void);
#endif
private:
	void begin(void);
	void update_16bit_22_mono(void);
	void update_16bit_22_stereo(void);
	void update_16bit_44_mono(void);
	void update_16bit_44_stereo(void);
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
	uint8_t* buffer;
#else
    uint8_t buffer[4*AUDIO_BLOCK_SAMPLES];     // buffer one block of data.
#endif
	audio_block_t *block_left;
	audio_block_t *block_right;
};


#endif // play_fs_wav_h_
