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

#ifndef synth_sine_h_
#define synth_sine_h_

//#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"

// TODO: investigate making a high resolution sine wave
// using Taylor series expansion.
// http://www.musicdsp.org/showone.php?id=13

class AudioSynthWaveformSine : public AudioStream
{
public:
	AudioSynthWaveformSine() : AudioStream(0, NULL), magnitude(16384) {}
	virtual void update(void);
	virtual bool is_idle(void);
	void frequency(float freq);
	void phase(float angle);
	void amplitude(float n);
private:
	uint32_t phase_accumulator;
	uint32_t phase_increment;
	int32_t magnitude;
};


class AudioSynthWaveformSineHires : public AudioStream
{
public:
	AudioSynthWaveformSineHires() : AudioStream(0, NULL), magnitude(16384) {}
	virtual void update(void);
	virtual bool is_idle(void);
	void frequency(float freq);
	void phase(float angle);
	void amplitude(float n);
private:
	uint32_t phase_accumulator;
	uint32_t phase_increment;
	int32_t magnitude;
};


class AudioSynthWaveformSineModulated : public AudioStream
{
public:
	AudioSynthWaveformSineModulated() : AudioStream(1, inputQueueArray), magnitude(16384) {}
	// maximum unmodulated carrier frequency is 11025 Hz
	// input = +1.0 doubles carrier
	// input = -1.0 DC output
	virtual void update(void);
	virtual bool is_idle(void);
	void frequency(float freq);
	void phase(float angle);
	void amplitude(float n);
private:
	uint32_t phase_accumulator;
	uint32_t phase_increment;
	audio_block_t *inputQueueArray[1];
	int32_t magnitude;
};


#endif
