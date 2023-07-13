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


static SemaphoreHandle_t audio_semaphore = NULL;
static StaticSemaphore_t audio_mutex_buffer;

void matched_rtos_semaphore_take(){
	// Lazy initialization of the mutex.
	if(audio_semaphore == NULL){
		audio_semaphore = xSemaphoreCreateRecursiveMutexStatic( &audio_mutex_buffer );
	}
	// It is okay to take a semaphore before the RTOS scheduler has started.
	xSemaphoreTakeRecursive( audio_semaphore, portMAX_DELAY );
}

void matched_rtos_semaphore_give(){
	// It is okay to take a semaphore before the RTOS scheduler has started.
	xSemaphoreGiveRecursive( audio_semaphore );
}
