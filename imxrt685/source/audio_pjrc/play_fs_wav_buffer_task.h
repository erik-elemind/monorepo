/*
 * wavbuf.h
 *
 *  Created on: Sep 29, 2020
 *      Author: David Wang
 */

#ifndef AUDIO_PJRC_WAVBUF_H_
#define AUDIO_PJRC_WAVBUF_H_

#include "FreeRTOS.h"
#include "task.h"
#include "config.h"

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

#if (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK))

// Init called before vTaskStartScheduler() launches our Task in main():
void wavbuf_pretask_init(void);

void wavbuf_task(void *ignored);

void wavbuf_task_wakeup();

#endif // (defined(ENABLE_WAVBUF_TASK) && (ENABLE_WAVBUF_TASK))

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif /* AUDIO_PJRC_WAVBUF_H_ */
