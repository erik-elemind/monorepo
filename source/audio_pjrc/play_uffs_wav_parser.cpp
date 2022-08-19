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

#include "play_uffs_wav_parser.h"
#include "utils.h"
#include "loglevels.h"
#include "stream_memory_rtos.h"


void AudioPlayUffsWavParser::begin_parser(void)
{
	stop_parser();
}

uint32_t AudioPlayUffsWavParser::get_audio_data_offset(void) {
  return file_audio_start_offset;
}

bool AudioPlayUffsWavParser::start_parser(void)
{
	stop_parser();
	buffer_length = 0;
	buffer_offset = 0;
	data_length = 20;
	header_offset = 0;
	state = STATE_PARSE1;
	file_audio_start_offset = 0;
	return true;
}

bool AudioPlayUffsWavParser::start_parser_at_audio_data_offset(void) {
  stop_parser();
  buffer_length = 0;   // is reset by parse();
  buffer_offset = 0;   // is reset by parse();
  data_length = total_length; // use length of audio data from previous play back of file.
  header_offset = 0;   // not used in any of the play states
  state = state_play;  // use state play of file.
  return true;
}

void AudioPlayUffsWavParser::stop_parser(void)
{
	state = STATE_STOP;
	data_length = 0;
}

void AudioPlayUffsWavParser::parse(uint8_t *buffer, uint32_t size, void* audio_out_buf_handle)
{
  buffer_length = size;
  buffer_offset = 0;

  while( buffer_offset < size ){
    int32_t n = buffer_length - buffer_offset;
    if (n > 0) {
        // we have buffered data
        consume(buffer, n, audio_out_buf_handle);
        if (state == STATE_STOP) return;
    }
  }
  if ( state >= STATE_PARSE1 ){
    file_audio_start_offset += size;
  }
}

// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

// Consume already buffered data.  Returns true if audio transmitted.
bool AudioPlayUffsWavParser::consume(uint8_t *buffer, uint32_t size, void* audio_out_buf_handle)
{
	uint32_t len;
	const uint8_t *p;

	p = buffer + buffer_offset;
start:
	if (size == 0) return false;
#if 0
	Serial.print("AudioPlaySdWav consume, ");
	Serial.print("size = ");
	Serial.print(size);
	Serial.print(", buffer_offset = ");
	Serial.print(buffer_offset);
	Serial.print(", data_length = ");
	Serial.print(data_length);
	Serial.print(", space = ");
	Serial.print((AUDIO_BLOCK_SAMPLES - block_offset) * 2);
	Serial.print(", state = ");
	Serial.println(state);
#endif
	switch (state) {
	  // parse wav file header, is this really a .wav file?
	  case STATE_PARSE1:
		len = data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);
		header_offset += len;
		buffer_offset += len;
		data_length -= len;
		if (data_length > 0) return false;
		// parse the header...
		if (header[0] == 0x46464952 && header[2] == 0x45564157) {
			//Serial.println("is wav file");
			if (header[3] == 0x20746D66) {
				// "fmt " header
				if (header[4] < 16) {
					// WAV "fmt " info must be at least 16 bytes
					break;
				}
				if (header[4] > sizeof(header)) {
					// if such .wav files exist, increasing the
					// size of header[] should accomodate them...
					//Serial.println("WAVEFORMATEXTENSIBLE too long");
					break;
				}
				//Serial.println("header ok");
				header_offset = 0;
				state = STATE_PARSE2;
			} else {
				// first chuck is something other than "fmt "
				//Serial.print("skipping \"");
				//Serial.printf("\" (%08X), ", __builtin_bswap32(header[3]));
				//Serial.print(header[4]);
				//Serial.println(" bytes");
				header_offset = 12;
				state = STATE_PARSE5;
			}
			p += len;
			size -= len;
			data_length = header[4];
			goto start;
		}
		//Serial.println("unknown WAV header");
		break;

	  // check & extract key audio parameters
	  case STATE_PARSE2:
		len = data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);
		header_offset += len;
		buffer_offset += len;
		data_length -= len;
		if (data_length > 0) return false;
		if (parse_format()) {
			//Serial.println("audio format ok");
			p += len;
			size -= len;
			data_length = 8;
			header_offset = 0;
			state = STATE_PARSE3;
			goto start;
		}
		//Serial.println("unknown audio format");
		break;

	  // find the data chunk
	  case STATE_PARSE3: // 10
		len = data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);
		header_offset += len;
		buffer_offset += len;
		data_length -= len;
		if (data_length > 0) return false;
		//Serial.print("chunk id = ");
		//Serial.print(header[0], HEX);
		//Serial.print(", length = ");
		//Serial.println(header[1]);
		p += len;
		size -= len;
		data_length = header[1];
		if (header[0] == 0x61746164) {
			//Serial.print("wav: found data chunk, len=");
			//Serial.println(data_length);
			// TODO: verify offset in file is an even number
			// as required by WAV format.  abort if odd.  Code
			// below will depend upon this and fail if not even.
			state = state_play;
			total_length = data_length;
			// About to start parsing audio data, so save off the
			// number of bytes read thus far
			file_audio_start_offset += (p-buffer);
		} else {
			state = STATE_PARSE4;
		}
		goto start;

	  // ignore any extra unknown chunks (title & artist info)
	  case STATE_PARSE4: // 11
		if (size < data_length) {
			data_length -= size;
			buffer_offset += size;
			return false;
		}
		p += data_length;
		size -= data_length;
		buffer_offset += data_length;
		data_length = 8;
		header_offset = 0;
		state = STATE_PARSE3;
		//Serial.println("consumed unknown chunk");
		goto start;

	  // skip past "junk" data before "fmt " header
	  case STATE_PARSE5:
		len = data_length;
		if (size < len) len = size;
		buffer_offset += len;
		data_length -= len;
		if (data_length > 0) return false;
		p += len;
		size -= len;
		data_length = 8;
		state = STATE_PARSE1;
		goto start;

	  // playing mono at native sample rate
	  case STATE_DIRECT_8BIT_MONO:
		return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_8BIT_STEREO:
		return false;

	  // playing mono at native sample rate
	  case STATE_DIRECT_16BIT_MONO:
	  {
		if (size > data_length) size = data_length;
		data_length -= size;

// Use this block to buffer mono audio as if it were stereo audio
#if 0
        // copy the data into a temporary buffer
		size_t bytes_written = 0;
		uint8_t* temp_p = (uint8_t*) p;
		while( true ) {
		  // batch the stream buffer sends
          uint8_t temp_buf[4*20];
          size_t  temp_buf_i = 0;
          while( temp_buf_i < 4*20 ) {
            uint8_t lsb = *temp_p++;
            uint8_t msb = *temp_p++;
            bytes_written += 2;
            // left channel
            temp_buf[temp_buf_i++] = lsb;
            temp_buf[temp_buf_i++] = msb;
            // right channel (duplicate of left channel)
            temp_buf[temp_buf_i++] = lsb;
            temp_buf[temp_buf_i++] = msb;
            if(size == bytes_written){
              goto all_data_sent;
            }
          }
          // assume all bytes are written
          xStreamBufferSend( audio_out_buf_handle, (void*) temp_buf, temp_buf_i, portMAX_DELAY);
		}

		all_data_sent:
#endif
// Use this block to buffer mono audio as is (as a mono audio stream)
#if 1

        void* data = (void*) p;
        size_t data_len = size;
        size_t bytes_written = 0;
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
        smem_rtos_write((smem_rtos_t*) audio_out_buf_handle, data, data_len);
        bytes_written = data_len;
#else
        StreamBufferHandle_t handle = *((StreamBufferHandle_t*)audio_out_buf_handle);
        bytes_written = xStreamBufferSend( handle, (void*) data, data_len, portMAX_DELAY);
#endif
#endif
        // increment p
        p += bytes_written;
        // compute the remaining 'size' bytes in buffer.
        size -= bytes_written;
        // add back whatever 'size' bytes was not written.
        data_length += size;
        // compute the new buffer_offset based on where it is now, versus where its 0-index is.
        buffer_offset = p - buffer;

		if (data_length == 0) {
		  state = STATE_STOP;
		}
		return true;
	  }

	  // playing stereo at native sample rate
	  case STATE_DIRECT_16BIT_STEREO:
	  {
		if (size > data_length) size = data_length;
		data_length -= size;

        void* data = (void*) p;
        size_t data_len = size;
        size_t bytes_written = 0;
#if (defined(ENABLE_NO_COPY_WAV_BUFFER) && (ENABLE_NO_COPY_WAV_BUFFER > 0U))
        smem_rtos_write((smem_rtos_t*) audio_out_buf_handle, data, data_len);
        bytes_written = data_len;
#else
        StreamBufferHandle_t handle = *((StreamBufferHandle_t*)audio_out_buf_handle);
        bytes_written = xStreamBufferSend( handle, (void*) data, data_len, portMAX_DELAY);
#endif
        // increment p
        p += bytes_written;
        // compute the remaining 'size' bytes in buffer.
        size -= bytes_written;
        // add back whatever 'size' bytes was not written.
        data_length += size;
        // compute the new buffer_offset based on where it is now, versus where its 0-index is.
        buffer_offset = p - buffer;

        if (data_length == 0) {
          state = STATE_STOP;
        }
        return true;
	  }
#if 0
	  // playing mono, converting sample rate
	  case STATE_CONVERT_8BIT_MONO :
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_8BIT_STEREO:
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_16BIT_MONO:
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_16BIT_STEREO:
		return false;
#endif
	  // ignore any extra data after playing
	  // or anything following any error
	  case STATE_STOP:
		return false;

	  // this is not supposed to happen!
	  //default:
		//Serial.println("AudioPlaySdWav, unknown state");
	}
	state = STATE_STOP;
	return false;
}


/*
00000000  52494646 66EA6903 57415645 666D7420  RIFFf.i.WAVEfmt 
00000010  10000000 01000200 44AC0000 10B10200  ........D.......
00000020  04001000 4C495354 3A000000 494E464F  ....LIST:...INFO
00000030  494E414D 14000000 49205761 6E742054  INAM....I Want T
00000040  6F20436F 6D65204F 76657200 49415254  o Come Over.IART
00000050  12000000 4D656C69 73736120 45746865  ....Melissa Ethe
00000060  72696467 65006461 746100EA 69030100  ridge.data..i...
00000070  FEFF0300 FCFF0400 FDFF0200 0000FEFF  ................
00000080  0300FDFF 0200FFFF 00000100 FEFF0300  ................
00000090  FDFF0300 FDFF0200 FFFF0100 0000FFFF  ................
*/





// SD library on Teensy3 at 96 MHz
//  256 byte chunks, speed is 443272 bytes/sec
//  512 byte chunks, speed is 468023 bytes/sec

#define B2M_44100 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT) // 97352592
#define B2M_22050 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 2.0)
#define B2M_11025 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 4.0)

bool AudioPlayUffsWavParser::parse_format(void)
{
	uint8_t num = 0;
	uint16_t format;
	uint16_t channels;
	uint32_t rate, b2m;
	uint16_t bits;

	format = header[0];
	//Serial.print("  format = ");
	//Serial.println(format);
	if (format != 1) return false;

	rate = header[1];

	//Serial.print("  rate = ");
	//Serial.println(rate);
	if (rate == 44100) {
		b2m = B2M_44100;
		sample_rate = 44100;
	} else if (rate == 22050) {
		b2m = B2M_22050;
		// Ignore setting the play state based on rate conversion
		// num |= 4;
		sample_rate = 22050;
	} else if (rate == 11025) {
		b2m = B2M_11025;
		// Ignore setting the play state based on rate conversion
		// num |= 4;
		sample_rate = 11025;
	} else {
	    sample_rate = 0;
		return false;
	}

	channels = header[0] >> 16;
	//Serial.print("  channels = ");
	//Serial.println(channels);
	if (channels == 1) {
	} else if (channels == 2) {
		b2m >>= 1;
		num |= 1;
	} else {
		return false;
	}

	bits = header[3] >> 16;
	//Serial.print("  bits = ");
	//Serial.println(bits);
	if (bits == 8) {
	} else if (bits == 16) {
		b2m >>= 1;
		num |= 2;
	} else {
		return false;
	}

	bytes2millis = b2m;
	//Serial.print("  bytes2millis = ");
	//Serial.println(b2m);

	// we're not checking the byte rate and block align fields
	// if they're not the expected values, all we could do is
	// return false.  Do any real wav files have unexpected
	// values in these other fields?
	state_play = num;
	return true;
}


bool AudioPlayUffsWavParser::isPlaying(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s < 8);
}

uint32_t AudioPlayUffsWavParser::positionMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t dlength = *(volatile uint32_t *)&data_length;
	uint32_t offset = tlength - dlength;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)offset * b2m) >> 32;
}


uint32_t AudioPlayUffsWavParser::lengthMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)tlength * b2m) >> 32;
}

