/*
 * timer_util.h
 *
 *  Created on: Oct 13, 2020
 *      Author: David Wang
 */

#ifndef UTILS_TIMER_UTIL_H_
#define UTILS_TIMER_UTIL_H_

#include "fsl_common.h" // needed for DWT

inline void enable_cycle_counter(){
  // Enable cycle counter
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable access to registers for data watchpoint and trace.
  DWT->CTRL = (1 << DWT_CTRL_CYCCNTENA_Pos); // Enable cycle counter
}

inline uint32_t cycles(){
  uint32_t totalcycles = DWT->CYCCNT;
  return totalcycles;
}


#endif /* UTILS_TIMER_UTIL_H_ */
