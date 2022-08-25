#ifndef mp3_h_
#define mp3_h_

//#include "Arduino.h"
#include "AudioStream.h"

#include "mp3dec.h"
#include "mp3common.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "ff.h"

/* Buffer sizes taken from 
 * https://github.com/adafruit/Adafruit_MP3/blob/master/src/Adafruit_MP3.cpp
 */
#define BUFFER_LOWER_THRESH 1024
#define INPUT_BUFFER_SIZE (2 * 1024)
#define OUTPUT_BUFFER_SIZE (4 * 1024)
#define TRANSFER_SIZE (sizeof((audio_block_t*)0)->data)

// Decoder task
void mp3_pretask_init(void);
void mp3_task(void *ignored);
void mp3_task_wakeup();

class Mp3 : public AudioStream
{
public:
  Mp3();
  ~Mp3();
  void open(const char* filename);
  void play();
  void stop();
  
  virtual void update();
  virtual bool is_idle();
  int decode_frame();
  bool is_playing();

  // Note that mp3's generally have to be encoded specially to support
  // looping, due to the way the window function works at the
  // beginning and end of the file.  See for more details:
  // https://www.compuphase.com/mp3/mp3loops.htm
  void set_loop(bool l) {
    loop = l;
  }
private:
  void fill_inbuf();

  bool loop = true;
  HMP3Decoder decoder;
  unsigned inbuf_left = 0;
  unsigned char* inbuf_ptr = NULL;
  unsigned char inbuf[INPUT_BUFFER_SIZE];
  FIL file;
  MP3FrameInfo frame;
  bool playing = false;
  bool decode_success;
};

extern Mp3 mp3output;

#endif
