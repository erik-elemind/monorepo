/*
 * hardfault.c
 *
 *  Created on: Jul 20, 2021
 *      Author: DavidWang
 */

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

#define HALT_IF_DEBUGGING()                              \
  do {                                                   \
    if ((*(volatile uint32_t *)0xE000EDF0) & (1 << 0)) { \
      __asm("bkpt 1");                                   \
    }                                                    \
  } while (0)

//// DefaultIntHandler is used for unpopulated interrupts
//static void DefaultIntHandler(void) {
//  __asm__("bkpt");
//  // Go into an infinite loop.
//  while (1)
//    ;
//}

//static void NMI_Handler(void) {
//  DefaultIntHandler();
//}


typedef struct __attribute__((packed)) ContextStateFrame {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t return_address;
  uint32_t xpsr;
} sContextStateFrame;

#define HARDFAULT_HANDLING_ASM(_x)               \
  __asm volatile(                                \
      "tst lr, #4 \n"                            \
      "ite eq \n"                                \
      "mrseq r0, msp \n"                         \
      "mrsne r0, psp \n"                         \
      "b my_fault_handler_c \n"                  \
                                                 )


__attribute__((optimize("O0")))
void my_fault_handler_c(sContextStateFrame *frame) {
  HALT_IF_DEBUGGING();

  // Logic for dealing with the exception. Typically:
  //  - log the fault which occurred for postmortem analysis
  //  - If the fault is recoverable,
  //    - clear errors and return back to Thread Mode
  //  - else
  //    - reboot system

  //
  // Example "recovery" mechanism for UsageFaults while not running
  // in an ISR
  //

  // reboot system
  NVIC_SystemReset();
  printf("I shouldn't be here oh no!!!!!\n");
  while (1) { }

#if 0 // TODO: implement more sophisticated fault handling
  volatile uint32_t *cfsr = (volatile uint32_t *)0xE000ED28;
  const uint32_t usage_fault_mask = 0xffff0000;
  const bool non_usage_fault_occurred = (*cfsr & ~usage_fault_mask) != 0;
  // the bottom 8 bits of the xpsr hold the exception number of the
  // executing exception or 0 if the processor is in Thread mode
  const bool faulted_from_exception = ((frame->xpsr & 0xFF) != 0);

  if (faulted_from_exception || non_usage_fault_occurred) {
    // For any fault within an ISR or non-usage faults let's reboot the system
	 // ( __NVIC_SystemReset)
    volatile uint32_t *aircr = (volatile uint32_t *)0xE000ED0C;
    *aircr = (0x05FA << 16) | 0x1 << 2;
    while (1) { } // should be unreachable
  }

  // If it's just a usage fault, let's "recover"
  // Clear any faults from the CFSR
  *cfsr |= *cfsr;
  // the instruction we will return to when we exit from the exception
  frame->return_address = (uint32_t)recover_from_task_fault;
  // the function we are returning to should never branch
  // so set lr to a pattern that would fault if it did
  frame->lr = 0xdeadbeef;
  // reset the psr state and only leave the
  // "thumb instruction interworking" bit set
  frame->xpsr = (1 << 24);
#endif
}

//void HardFault_Handler(void) {
//  printf("HardFault handler!\n");
//  HARDFAULT_HANDLING_ASM();
//}

void MemManage_Handler(void){
  printf("MemManage handler!\n");
  HARDFAULT_HANDLING_ASM();
}

//void BusFault_Handler(void){
//  printf("BusFault handler!\n");
//  HARDFAULT_HANDLING_ASM();
//}
//
//void UsageFault_Handler(void){
//  printf("UsageFault_Handler!\n");
//  HARDFAULT_HANDLING_ASM();
//}

//void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName ){
//	printf("vApplicationStackOverflowHook!\n");
//	HARDFAULT_HANDLING_ASM();
//}

void vApplicationMallocFailedHook( void ){
	printf("vApplicationMallocFailedHook!\n");
  HARDFAULT_HANDLING_ASM();
}

