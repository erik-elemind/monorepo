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

#if 0
#define AUDIO_ENTER_CRITICAL __disable_irq
#define AUDIO_EXIT_CRITICAL __enable_irq
#elif 0
#define AUDIO_ENTER_CRITICAL  taskENTER_CRITICAL
#define AUDIO_EXIT_CRITICAL  taskEXIT_CRITICAL
#else
#define AUDIO_ENTER_CRITICAL void
#define AUDIO_EXIT_CRITICAL void
#endif


#ifdef __cplusplus
}
#endif


#endif /* AUDIO_PJRC_CRITICAL_SECTION_H_ */
