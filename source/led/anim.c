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

#include <string.h>
#include <limits.h>
#include "anim.h"

#include <stdio.h>
#include <string.h>

static anim_channel_t channels[ANIM_CHANNEL_COUNT];
static void anim_channel_tick(anim_channel_t *channel);


#ifdef FREERTOS_CONFIG_H
#include "timers.h"


static TimerHandle_t anim_timer;
static StaticTimer_t anim_timer_struct;

static void anim_timer_init(void)
{
  configASSERT(anim_timer == NULL);
  anim_timer = xTimerCreateStatic("animation timer",
                            pdMS_TO_TICKS(1000/ANIM_FPS),
                            pdTRUE,  // AutoReload == true
                            NULL,
                            anim_tick,
                            &(anim_timer_struct));

  configASSERT(anim_timer != NULL);
}

static void anim_timer_start(void)
{
  // Restart the timer, even if it was dormant:
  xTimerReset(anim_timer, portMAX_DELAY);
}

static void anim_timer_stop(void)
{
  xTimerStop(anim_timer, portMAX_DELAY);
}

int anim_get_current_time_in_ms(void)
{
  return (int)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

#else

// The timer functions are all no-ops. It is up to the user to call
// anim_tick() repeatedly (at least ANIM_FPS times per second for smooth results).
#define anim_timer_init(x)
#define anim_timer_start(x)
#define anim_timer_stop(x)

// This is necessary.
int anim_get_current_time_in_ms(void)
{
#error "Please implement this function for your environment/BSP/HAL library"
}

#endif  // FREERTOS_CONFIG_H


void anim_init(void)
{
  memset(channels, 0x0, sizeof(channels));
  anim_timer_init();
}

// This #define determines interpolation resolution, from 0 to this value.
// A value of 100 provides a percentage (0..100), which is fine for most applications.
// For a minor performance bump, using bitshifting instead of division, we use 256:
#define ANIM_INTERPOLATION_DIVISOR 256
//#define ANIM_INTERPOLATION_DIVISOR 65536

// LERP interpolation (optionally using bitshifting for the 0..256 range):
static int interpolate(int y1, int y0, int position)
{
  int result;
#if ANIM_INTERPOLATION_DIVISOR==256
  // (x >> 8) happes to be the same as (x / 256), but faster on most CPUs:
  result = (((y0 * position) >> 8) + ((y1 * (256 - position)) >> 8));
#else
  result = (y0 * position)/ANIM_INTERPOLATION_DIVISOR + 
         (y1 * (ANIM_INTERPOLATION_DIVISOR - position))/ANIM_INTERPOLATION_DIVISOR;
#endif
  return result;
}

void anim_tick(TimerHandle_t ignored)
{
  int all_channels_are_stopped = TRUE;
  
  for (int index = 0; index < ANIM_CHANNEL_COUNT; index++) {
    anim_channel_t *channel = &(channels[index]);
    
    if (channel->is_running == TRUE) {
      all_channels_are_stopped = FALSE;
      anim_channel_tick(channel);
    }
  }
  
  if (all_channels_are_stopped == TRUE) {
    // Shut down the timer (if any) to allow for battery savings.
    anim_timer_stop();
  }
}

void anim_channel_stop(int channel_index)
{
  if (channel_index >= ANIM_CHANNEL_COUNT) { return; }
  channels[channel_index].is_running = FALSE;
}

void anim_channel_stop_all(void)
{
  for (int index = 0; index < ANIM_CHANNEL_COUNT; index++) {
    anim_channel_stop(index);
  }
  // Shut down the timer (if any) to allow for battery savings.
  anim_timer_stop();
}

void anim_channel_start(int channel_index, const animation_t *animations, 
                        int animation_count, int is_looping)
{
  if (channel_index >= ANIM_CHANNEL_COUNT) { return; }

  anim_channel_t *channel = &(channels[channel_index]);

  channel->animations = animations;
  channel->animation_count = animation_count;
  channel->is_looping = is_looping;
  // Start at the first animation, with t0 set to now
  channel->animation_index = 0;

  channel->t0_in_ms = anim_get_current_time_in_ms();
  channel->is_running = TRUE;

  // Start the timer, if it's not already running:
  anim_timer_start();
}

int anim_total_time_ms(animation_t *animations, int animation_count) {
    int i=0;
    int total_time_ms = 0;
    
    for(i=0; i < animation_count; i++) {
        total_time_ms += animations[i].duration_in_ms;
    }
    
    return total_time_ms;
}


int anim_channel_time_remaining_ms(int channel_index) {
    int i=0;
    int time_remaining_ms = 0;

    // Get the currently-running animation entry:
    anim_channel_t *channel = &(channels[channel_index]);

    if (channel->is_running == TRUE) {    
        // First, get time remaining in the current animation...
        const animation_t *running_animation = &(channel->animations[channel->animation_index]);
        int delta_t = anim_get_current_time_in_ms() - channel->t0_in_ms;

        time_remaining_ms += running_animation->duration_in_ms - delta_t;
        
        // ...then add up time for any animations remaining on this channl:
        for(i=channel->animation_index+1; i < channel->animation_count; i++) {
            time_remaining_ms += channel->animations[i].duration_in_ms;
        }
    }
    
    return time_remaining_ms;
}



static void anim_channel_tick(anim_channel_t *channel)
{  
  int delta_t = anim_get_current_time_in_ms() - channel->t0_in_ms;
  // Get the currently-running animation entry:
  const animation_t *animation = &(channel->animations[channel->animation_index]);
  int new_value;
  int position;
  
  // Special handling for zero-time animations that only set a new value:
  if (animation->duration_in_ms == 0) {
    // Just set the desired (to) value (and prevent DivideByZero):
    new_value = animation->to;
  } else {
    // Clip delta_t, in case it exceeded duration_in_ms (needed for the math):
    delta_t = MIN(delta_t, animation->duration_in_ms);
    // Get the interpolated value for the current time:
    position = (ANIM_INTERPOLATION_DIVISOR * delta_t) / (animation->duration_in_ms);
    new_value = interpolate(animation->from, animation->to, position);
  }

  anim_set_value(new_value, animation->user_data);
  
  if (delta_t >= animation->duration_in_ms) {
    // The current animation finished. Move on to the next one, if any:
    channel->t0_in_ms += animation->duration_in_ms;
    channel->animation_index++;

    // Is the sequence of animations completed?
    if (channel->animation_index >= channel->animation_count) {
      if (channel->is_looping == TRUE) {
        // Restart from the beginning:
        channel->animation_index = 0;
      } else {
        // Done.
        channel->is_running = FALSE;
      }
    } else {
      // We have a new animation waiting to run. Kick it off immediately,
      // (to prevent lagging if duration_in_ms is less than 1/ANIM_FPS).
      anim_channel_tick(channel);
    }
  }
}
