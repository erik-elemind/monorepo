/*
 * AudioCompat.h
 *
 *  Created on: Sep 21, 2020
 *      Author: David Wang
 */

#ifndef AUDIO_PJRC_AUDIOCOMPAT_H_
#define AUDIO_PJRC_AUDIOCOMPAT_H_

#ifndef F_CPU
#if defined(CPU_LPC55S69JBD100)
#define F_CPU 150000000
#elif defined(CPU_MIMXRT685SFVKB)
#define F_CPU 250000000
#else
#error Must define CPU type.
#endif
#endif

#ifndef ARM_ARCH_8M_MAIN
#if defined(CPU_LPC55S69JBD100)
#define __ARM_ARCH_8M_MAIN__ 1
#elif defined(CPU_MIMXRT685SFVKB)
#define __ARM_ARCH_8M_MAIN__ 1
#else
#error Must define CPU type.
#endif
#endif

#endif /* AUDIO_PJRC_AUDIOCOMPAT_H_ */
