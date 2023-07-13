/*
 * audio_compute_task.h
 *
 *  Created on: Mar 9, 2023
 *      Author: DavidWang
 */

#ifndef AUDIO_STREAM_TASK_H_
#define AUDIO_STREAM_TASK_H_

#include "config.h"

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

void audio_stream_pretask_init(void);
void audio_stream_task(void *ignored);

void audio_stream_end(void);
void audio_stream_begin(void);

// Run the audio processing loop for all AudioStream instances.
// (This is invoked by I2S DMA Complete interrupt.)
void audio_stream_update(void);

// This is invoked by the I2S DMA interrupt in interrupt context:
void audio_stream_update_from_isr(status_t i2s_completion_status);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif /* AUDIO_STREAM_TASK_H_ */
