/*
 * audio.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jul, 2020
 * Author:  David Wang
 *
 * Description: Implements the audio task which
 *               controls the SSM2518 audio amplifier
 *               to plays sine waves and audio files.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>
#include <stdlib.h>
#include "fsl_common.h"
#include "config.h"

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

// Default amount of increment/decrement used for
// "audio_volume_up()" and "audio_volume_down()" functions.
#if !defined(AUDIO_VOLUME_STEP) || (AUDIO_VOLUME_STEP<1)
#define AUDIO_VOLUME_STEP 2
#endif

#ifndef AUDIO_ENABLE_FG_WAV
#define AUDIO_ENABLE_FG_WAV 0
#endif

#ifndef AUDIO_ENABLE_BG_WAV
#define AUDIO_ENABLE_BG_WAV ENABLE_WAVBUF_TASK
#endif

#ifndef AUDIO_ENABLE_PINK
#define AUDIO_ENABLE_PINK 1
#endif

#ifndef AUDIO_ENABLE_SINE
#define AUDIO_ENABLE_SINE 1
#endif

#ifndef AUDIO_ENABLE_MP3
#define AUDIO_ENABLE_MP3 ENABLE_AUDIO_MP3_TASK
#endif

#ifndef AUDIO_ENABLE_I2S_OUTPUT
#define AUDIO_ENABLE_I2S_OUTPUT 1
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void audio_pretask_init(void);

void audio_task(void *ignored);

/*****************************************************************************/
// Send various event types to this task:

/*
 * Power on the SSM2518.
 * Sets shutdown pin to HIGH and initializes the SSM2518.
 * The SSM2518 MUST be powered on in order to use any other
 * function.
 */
void audio_power_on();

/*
 * Power off the SSM2518.
 * Sets shutdown pin to LOW.
 */
void audio_power_off();

/*
 * Stop playing any audio source.
 */
void audio_stop();

/*
 * Pause audio file playback
 * Only works when the audio file is playing.
 */
void audio_pause();

/*
 * Pause audio file playback
 * Only works when the audio file is playing.
 */
void audio_unpause();

/*
 * Mute and unmute the audio
 */
void audio_set_mute(bool m);
bool audio_get_mute();

/*
 * Get and set volume
 * 0   = mute
 * 255 = full volume
 */
void audio_set_volume(uint8_t log_volume);
void audio_set_volume_ble(uint8_t log_volume);
void audio_get_volume(uint8_t* log_volume, uint8_t* lin_volume);

/*
 * Increment/Decrement the volume.
 */
// Sets the step-size used by "audio_volume_up()" and "audio_volume_down()".
void audio_set_volume_step(uint8_t step);
// Increases the volume by 'step' size set by function "audio_set_volume_step()".
// If volume+step > 255, volume = 255.
void audio_volume_up();
// Decreases the volume by 'step' size set by function "audio_set_volume_step()".
// If volume-step < 0, volume = 0.
void audio_volume_down();

/*
 * Plays a wav audio file.
 */
void audio_fgwav_play(char* filename, bool loop);
void audio_fgwav_stop();
void audio_fg_fadein(uint32_t dur_ms);
void audio_fg_fadeout(uint32_t dur_ms);

/*
 * Wav audio file.
 */
void audio_bgwav_play(char* filename, bool loop);
void audio_bgwav_stop();
void audio_bg_fadein(uint32_t dur_ms);
void audio_bg_fadeout(uint32_t dur_ms);
void audio_bg_script_volume(float gain);   // uses critical section to update gain immediately.
void audio_bg_computed_volume(float gain); // uses critical section to update gain immediately.
void audio_bg_default_volume();

/*
 * MP3 audio file.
 */
void audio_mp3_play(const char* filename, bool loop);
void audio_mp3_stop();
void audio_mp3_fadein(uint32_t dur_ms);
void audio_mp3_fadeout(uint32_t dur_ms);

/*
 * Pink noise.
 */
void audio_pink_play();
void audio_pink_stop();
void audio_pink_fadein(uint32_t dur_ms);
void audio_pink_fadeout(uint32_t dur_ms);
void audio_pink_script_volume(float gain); // uses critical section to update gain immediately.
void audio_pink_computed_volume(float gain); // uses critical section to update gain immediately.
void audio_pink_mute(bool mute); // uses critical section to update gain immediately.
void audio_pink_default_volume();

/*
 * Plays a test sine wave.
 * Left channel - plays a 440 Hz sine wave.
 * Right channel - plays a 880 Hz sine wave.
 */
void audio_sine_play();
void audio_sine_stop();

// Run the audio processing loop for all AudioStream instances.
// (This is invoked by I2S DMA Complete interrupt.)
void audio_event_update_streams(void);

// This is invoked by the I2S DMA interrupt in interrupt context:
void audio_event_update_streams_from_isr(status_t i2s_completion_status);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif  // AUDIO_H
