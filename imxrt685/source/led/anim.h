/*
  The MIT License (MIT)

  Copyright (c) 2015 Derek Simkowiak <derek@simkowiak.net>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
  OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __ANIM_H__
#define __ANIM_H__

// Uncomment this line to enable FreeRTOS support.
//#include <FreeRTOS.h>
#include "FreeRTOS.h"
#include "task.h"
#include "utils.h"

// The number of animations that can be running at once. ~20 bytes RAM each:
#define ANIM_CHANNEL_COUNT 3

// Animation update Frames Per Second (in Hz).
// 8-bit machines should use a higher value, like 120, to clip delta_t to a small number.
// 32-bit machines should set it lower, to conserve battery and minimize interrupts.
// If animating LEDs, anything above 24 looks fairly smooth to the human eye.
#define ANIM_FPS 24


typedef struct
{
  int from;
  int to;
  int duration_in_ms;
  void *user_data;
} animation_t;

// A channel runs through a list of timed animations, possibly repeating at the end.
typedef struct
{
  const animation_t *animations;  // Pointer to the variable-length array of animations
  int animation_count;      // sizeof(animations)/sizeof(animations[0])
  int animation_index;      // Currently-running animation
  int is_looping;           // Attribute, whether or not to loop at the end.
  int is_running;           // Status variable
  int t0_in_ms;             // Current run's start time (t0) in ms
} anim_channel_t;


void anim_init(void);

// channel_index must be 0..(ANIM_CHANNEL_COUNT-1). It is an array index.
void anim_channel_start(int channel_index, const animation_t *animations, 
                        int animation_count, int is_looping);

void anim_channel_stop(int channel_index);

// Convenience function:
void anim_channel_stop_all(void);


int anim_total_time_ms(animation_t *animations, int animation_count);
int anim_channel_time_remaining_ms(int channel_index);


// Update all running animations.
//
// This can be called as a bare-metal task tick, in a while(1) main loop.
// Or it can be used as a timer-triggered callback (e.g. with FreeRTOS's  
// xTimerCreate(), or from a timer interrupt handler).
//
// It is also used by anim_task() if using FreeRTOS with a dedicated thread.
//
// NOTE: void *ignored is not used, it is only here for compatibility 
// with xTimerCreate()
void anim_tick(TimerHandle_t ignored);

// Callback function to set an interpolated value --
// This must be defined by the user in a .c file somewhere.
extern void anim_set_value(int value, void *user_data);


#endif  // __ANIM_H__
