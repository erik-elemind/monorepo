/*
 * critical_section.c
 *
 *  Created on: Jul 15, 2021
 *      Author: DavidWang
 */

//#include "core_cm33.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#if 0

static volatile uint32_t ulCriticalNesting = 0;

void matched_disable_irq( void ) /* PRIVILEGED_FUNCTION */
{
    __disable_irq();
    ulCriticalNesting++;

//    /* Barriers are normally not required but do ensure the code is
//     * completely within the specified behaviour for the architecture. */
//    __asm volatile( "dsb" ::: "memory" );
//    __asm volatile( "isb" );
}
/*-----------------------------------------------------------*/

void matched_enable_irq( void ) /* PRIVILEGED_FUNCTION */
{
    configASSERT( ulCriticalNesting );
    ulCriticalNesting--;

    if( ulCriticalNesting == 0 )
    {
      __enable_irq();
    }
}

#endif


static SemaphoreHandle_t xSemaphore = NULL;
static StaticSemaphore_t xMutexBuffer;

void matched_rtos_semaphore_init(){
	xSemaphore = xSemaphoreCreateRecursiveMutexStatic( &xMutexBuffer );
}

void matched_rtos_semaphore_take(){
	if(xSemaphore){
		xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY );
	}
}

void matched_rtos_semaphore_give(){
	if(xSemaphore){
		xSemaphoreGiveRecursive( xSemaphore );
	}
}
