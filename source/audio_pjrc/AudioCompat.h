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
//#define F_CPU 100000000
#endif
#endif

#ifndef ARM_ARCH_8M_MAIN
#if defined(CPU_LPC55S69JBD100)
#define __ARM_ARCH_8M_MAIN__ 1
#endif
#endif

#endif /* AUDIO_PJRC_AUDIOCOMPAT_H_ */
