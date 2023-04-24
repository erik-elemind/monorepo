/*
 * critical_section.h
 *
 *  Created on: Jul 15, 2021
 *      Author: DavidWang
 */

#ifndef AUDIO_PJRC_CRITICAL_SECTION_H_
#define AUDIO_PJRC_CRITICAL_SECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

//void matched_disable_irq();
//void matched_enable_irq();

void matched_rtos_semaphore_init();
void matched_rtos_semaphore_take();
void matched_rtos_semaphore_give();

#if 0
#define AUDIO_ENTER_CRITICAL __disable_irq
#define AUDIO_EXIT_CRITICAL __enable_irq
#elif 0
#define AUDIO_ENTER_CRITICAL  taskENTER_CRITICAL
#define AUDIO_EXIT_CRITICAL  taskEXIT_CRITICAL
#elif 0
#define AUDIO_ENTER_CRITICAL void
#define AUDIO_EXIT_CRITICAL void
#else
#define AUDIO_ENTER_CRITICAL matched_rtos_semaphore_take
#define AUDIO_EXIT_CRITICAL matched_rtos_semaphore_give
#endif


#ifdef __cplusplus
}
#endif


#endif /* AUDIO_PJRC_CRITICAL_SECTION_H_ */
