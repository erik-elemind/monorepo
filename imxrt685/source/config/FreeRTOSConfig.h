/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "MIMXRT685S_cm33.h"
#include "config_tracealyzer.h"
#include "memfault/ports/freertos_trace.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Declare toggle debug LED function here instead of #including
   board_config.h to avoid include file cycle. */
extern void BOARD_ToggleDebugLED(void);
#ifdef __cplusplus
}
#endif

// Bugfix for Race Condition in stream_buffer.c:
// See: https://github.com/aws/amazon-freertos/issues/1837
#ifndef sbSEND_COMPLETED
	#define sbSEND_COMPLETED( pxStreamBuffer )											\
		vTaskSuspendAll();																\
		{																				\
			if( ( pxStreamBuffer )->xTaskWaitingToReceive != NULL && 						\
  			  prvBytesInBuffer( pxStreamBuffer ) >= pxStreamBuffer->xTriggerLevelBytes) \
			{																			\
				( void ) xTaskNotify( ( pxStreamBuffer )->xTaskWaitingToReceive,		\
									  ( uint32_t ) 0,									\
									  eNoAction );										\
				( pxStreamBuffer )->xTaskWaitingToReceive = NULL;						\
			}																			\
		}																				\
		( void ) xTaskResumeAll();
#endif /* sbSEND_COMPLETED */


/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION                    1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      ((TickType_t)200)
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ((unsigned short)128) //((unsigned short)256)
#define configMAX_TASK_NAME_LEN                 20
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_ALTERNATIVE_API               0 /* Deprecated! */
#define configQUEUE_REGISTRY_SIZE               30
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  0
#define configUSE_NEWLIB_REENTRANT              1
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 1
#define configUSE_APPLICATION_TASK_TAG          0

/* Used memory allocation (heap_x.c) */
// #define configFRTOS_MEMORY_SCHEME               4
/* Tasks.c additions (e.g. Thread Aware Debug capability) */
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 1

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
// #define configTOTAL_HEAP_SIZE                   ((size_t)(90112)) // Not used by heap_Newlib.c
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS           0
#if 1
#include "micro_clock.h"
// Choose to use a more precise clock for runtime stats
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  init_micro_clock
#define portGET_RUN_TIME_COUNTER_VALUE          micros
#else
// Or, use the less precise SysTick timer rather than a dedicated hardware timer:
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  xTaskGetTickCount
#define portGET_RUN_TIME_COUNTER_VALUE          xTaskGetTickCount
#endif

#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Task aware debugging. */
#define configRECORD_STACK_HIGH_ADDRESS         1

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2

/* Software timer related definitions. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                20
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)
#define configTIMER_SERVICE_TASK_NAME    "Tmr Svc"

/* IDLE task related definitions.  */
#define configIDLE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE*2)
#define configIDLE_TASK_NAME    "IDLE"

//#define configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES  1




//#define configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES  1


void vAssertCalled(const char *file, int line);
#define configASSERT(x) if ((x) == 0){ vAssertCalled( __FILE__, __LINE__ );}


/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   0
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1



#if defined(__ICCARM__)||defined(__CC_ARM)||defined(__GNUC__)
    /* Clock manager provides in this variable system core clock frequency */
    #include <stdint.h>
    extern uint32_t SystemCoreClock;
#endif


#ifndef configENABLE_FPU
  #define configENABLE_FPU                      1
#endif
#ifndef configENABLE_MPU
  #define configENABLE_MPU                      0
#endif
#ifndef configENABLE_TRUSTZONE
  #define configENABLE_TRUSTZONE                0
#endif
#ifndef configRUN_FREERTOS_SECURE_ONLY
  #define configRUN_FREERTOS_SECURE_ONLY        1
#endif

/* Interrupt nesting behaviour configuration. Cortex-M specific. */
#ifdef __NVIC_PRIO_BITS
/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 3 /* 7 priority levels */
#endif

// TODO: The following #define was added for FF4, in comparison to FreeRTOS_hello example for the RT685.
// This #define did not exist for FF3.
#define configSTACK_DEPTH_TYPE uint32_t

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY ((1U << (configPRIO_BITS)) - 1)

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler SVC_Handler
#define vPortPendSVHandler PendSV_Handler
#define vPortSysTickHandler SysTick_Handler


/* Map the FreeRTOS printf() to the logging task printf. */
#define configPRINTF( x )          vLoggingPrintf x

/* Map the logging task's printf to the board specific output function. */
#define configPRINT_STRING

/* Sets the length of the buffers into which logging messages are written - so
 * also defines the maximum length of each log message. */
#define configLOGGING_MAX_MESSAGE_LENGTH            256

/* Set to 1 to prepend each log message with a message number, the task name,
 * and a time stamp. */
#define configLOGGING_INCLUDE_TIME_AND_TASK_NAME    1

#if (defined(ENABLE_TRACEALYZER) && (ENABLE_TRACEALYZER > 0U))
#include "trcRecorder.h" /* Should be in the end, after the #define. */
#endif

#endif /* FREERTOS_CONFIG_H */
