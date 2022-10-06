/*
 * util_delay.c
 *
 *  Created on: Oct 5, 2022
 *      Author: tyler
 */
#include "util_delay.h"

extern uint32_t SystemCoreClock;

void util_delay_ms(uint16_t delay_ms){
	SDK_DelayAtLeastUs(delay_ms * 1000, SystemCoreClock);
}
