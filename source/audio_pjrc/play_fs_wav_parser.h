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

#ifndef play_uffs_wav_parser_h_
#define play_uffs_wav_parser_h_

#include "AudioStream.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "config.h"


#define STATE_DIRECT_8BIT_MONO      0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO    1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO     2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO   3  // playing stereo at native sample rate
//#define STATE_CONVERT_8BIT_MONO     4  // playing mono, converting sample rate
//#define STATE_CONVERT_8BIT_STEREO   5  // playing stereo, converting sample rate
//#define STATE_CONVERT_16BIT_MONO    6  // playing mono, converting sample rate
//#define STATE_CONVERT_16BIT_STEREO  7  // playing stereo, converting sample rate
#define STATE_PARSE1            8  // looking for 20 byte ID header
#define STATE_PARSE2            9  // looking for 16 byte format header
#define STATE_PARSE3            10 // looking for 8 byte data header
#define STATE_PARSE4            11 // ignoring unknown chunk after "fmt "
#define STATE_PARSE5            12 // ignoring unknown chunk before "fmt "
#define STATE_STOP          13


class AudioPlayFsWavParser
{
public:
  AudioPlayFsWavParser(void) { begin_parser(); }
	void begin_parser(void);
	bool start_parser(void);
    bool start_parser_at_audio_data_offset(void);
	void stop_parser(void);
	bool isPlaying(void);
	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
    void parse(uint8_t *buffer, uint32_t size, void* audio_out_buf_handle);
    void get_stream_params(uint8_t &state_play, uint32_t &sample_rate){
      // TODO: This should probably be protected by a semaphore.
      state_play = this->state_play;
      sample_rate = this->sample_rate;
    }
    uint32_t get_audio_data_offset(void);
private:
    bool consume(uint8_t *buffer, uint32_t size, void* audio_out_buf_handle);
	bool parse_format(void);
    uint32_t header[10];        // temporary storage of wav header data
    uint32_t data_length;       // number of bytes remaining in current section
    uint32_t total_length;      // number of audio data bytes in file
    uint32_t bytes2millis;      // conversion multiple from wav file data bytes to millis ellapsed play time.
    uint16_t block_offset;      // how much data is in block_left & block_right
    uint16_t buffer_offset;     // where we're at consuming "buffer"
    uint16_t buffer_length;     // how much data is in "buffer" (512 until last read)
    uint8_t header_offset;      // number of bytes in header[]
    uint8_t state;
    uint8_t state_play;         // the number of channels and bits of the MOST RECENTLY parsed wav file.
    uint32_t sample_rate;       // the sample rate in Hz           of the MOST RECENTLY parsed WAV file.
    uint32_t file_audio_start_offset; // number of bytes in the file before audio data starts.

};

#endif // play_uffs_wav_parser_h_
