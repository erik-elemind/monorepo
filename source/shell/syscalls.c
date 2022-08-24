
//
// The C library newlib-nano has several low-level system call functions
// which are weakly linked, meaning, you can override the default implementation
// by redefining your own version of those functions. The conventional file name
// for customized system calls is "syscalls.c". That file is sometimes
// defined by an IDE or toolchain, and there are many example "syscalls.c" files
// shipped with STM32Cube, but here we define it manually.
//
#include "FreeRTOS.h"
#include "board_ff4.h"
#include "task.h"
#include <unistd.h>
#include "config.h"
#include <stdio.h>
#include <virtual_com.h>
#include "semphr.h"
#include "utils.h"
#include "syscalls.h"

#if defined(CONFIG_SHELL_USB)
#define DEFAULT_SYSCALL_WRITE_LOC SYSCALL_USB
#define DEFAULT_SYSCALL_READ_LOC  SYSCALL_USB
#elif defined(CONFIG_SHELL_UART)
#define DEFAULT_SYSCALL_WRITE_LOC SYSCALL_DEBUG_UART
#define DEFAULT_SYSCALL_READ_LOC  SYSCALL_DEBUG_UART
#endif

#ifndef DEFAULT_SYSCALL_WRITE_LOC
  #define DEFAULT_SYSCALL_WRITE_LOC SYSCALL_NONE
#endif

#ifndef DEFAULT_SYSCALL_READ_LOC
  #define DEFAULT_SYSCALL_READ_LOC  SYSCALL_NONE
#endif

uint32_t g_syscall_write_loc = DEFAULT_SYSCALL_WRITE_LOC;
uint32_t g_syscall_read_loc  = DEFAULT_SYSCALL_READ_LOC;

void syscalls_set_write_loc(uint32_t write_loc){
  g_syscall_write_loc = write_loc;
}

uint32_t syscalls_get_write_loc(){
  return g_syscall_write_loc;
}

void syscalls_set_read_loc(uint32_t read_loc){
  g_syscall_read_loc = read_loc;
}

uint32_t syscalls_get_read_loc(){
  return g_syscall_read_loc;
}

//
// Treat &SHELL_UART_HANDLE as stdin, stdout, and stderr.
//
// A more complex implementation could have a different UART for STDERR, or
// could map the int file argument to an open FatFs file handle and
// FatFS operations. But that would also require implementing _open(), _close(),
// etc. (It's less code to just use the FatFs API directly.)
//

//
// Many threads will try to write simultaneously, so we mutex the UART.
//
#ifndef CONFIG_USE_SEMIHOSTING
#if 0
// This is to take control of the UART without threads stomping on each other.
static SemaphoreHandle_t g_uart_semaphore_handle = NULL;
#endif

// This must be called before any read or write:
void syscalls_pretask_init(void)
{
  // To use _read() via a blocking getchar() in shell.c, it is necessary to first
  // turn off stdin buffering, so keystrokes show up immediately:
  // WARNING: This is ignored by newlib-nano, so a manual fflush() is needed in _read().
  setvbuf(stdin, NULL, _IONBF, 0);   // (during init)
  setvbuf(stdout, NULL, _IONBF, 0);  // (during init)
  setvbuf(stderr, NULL, _IONBF, 0);  // (during init)

#if 0
  g_uart_semaphore_handle = xSemaphoreCreateMutex();
  vQueueAddToRegistry(g_uart_semaphore_handle, "syscalls_sem");
#endif
}

int _write(int file, char *ptr, int len)
{
  int result = -1;
#if 0
  BaseType_t schedulerState = xTaskGetSchedulerState();

  if (schedulerState != taskSCHEDULER_NOT_STARTED) {
    xSemaphoreTake( g_uart_semaphore_handle, portMAX_DELAY );
  }
#endif

  if (g_syscall_write_loc & SYSCALL_DEBUG_UART){
    /* Don't use USART_RTOS_Send() -> USART_TransferSendNonBlocking() due to
      race condition in USART_TransferHandleIRQ(). */
    status_t status = USART_WriteBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t*)ptr, len);
    if (status == kStatus_Success) {
      result = len;
    }
  }
  if (g_syscall_write_loc & SYSCALL_USB){
    if(virtual_com_attached()){
      // If the com port is attached, then write over the com port.

        // write the virtual com
        result = virtual_com_write(ptr, len);
        // check the result
        if(result == len){
  //        debug_uart_puts("syscalls _write: SUCCESS WRITING");
        }else{
  //        debug_uart_puts("syscalls _write: ERROR WRITING");
        }
    }


    // TODO: Replace this with a call to USART NON Write Blocking
//    if(result!=len){
//      // Otherwise, write over the Debug UART.
//      status_t status = USART_WriteBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t*)ptr, len);
//      if (status == kStatus_Success) {
//        result = len;
//      }
//    }


  }
//  if (g_syscall_read_loc == SYSCALL_NONE){
//    #error "Morpheus debug shell device not specified!"
//  }

#if 0
  if (schedulerState != taskSCHEDULER_NOT_STARTED) {
    xSemaphoreGive( g_uart_semaphore_handle );
  }
#endif
  return result;
}

int _read(int file, char *ptr, int len)
{
  int result = -1;

  // To use _read() via a blocking getchar() in shell.c, it is necessary to first
  // turn off stdin buffering, so keystrokes show up immediately:
  // WARNING: This is ignored by newlib-nano, so a manual fflush() is needed.
  //setvbuf(stdin, NULL, _IONBF, 0);  // (during init)

  // newlib-nano doesn't respect setvbuf() for output and instead forces
  // `\n`-terminated line buffering. So to keep single-char keystrokes printing:
  fflush(stdout);
  fflush(stderr);

  if (file == STDIN_FILENO) {
    // UART reads must be clipped to a single character to be interactive:
    len = 1;

// This check on xTaskGetSchedulerState() is only necessary if we call
// getchar()/read() outside of a task. We use shell.c and so don't need it.
#ifdef SYSCALLS_READ_IN_MAIN
    // The vendor's USART_RTOS_*() API requires the scheduler to be running.
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
      status = USART_ReadBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t *)ptr, len);
      if (status == kStatus_Success) {
        result = 1;  // one byte successful read.
      }
      return result;
    }
#endif

    if (g_syscall_read_loc & SYSCALL_DEBUG_UART){
      status_t status;
      size_t received_count;
      status = USART_RTOS_Receive(&USART_DEBUG_RTOS_HANDLE, (uint8_t *)ptr, len, &received_count);
      if (status == kStatus_Success && received_count > 0) {
        result = len;
      }
    }
    if (g_syscall_read_loc & SYSCALL_USB){
      result = virtual_com_read(ptr, len);
    }
//    if (g_syscall_read_loc == SYSCALL_NONE){
//      #error "Morpheus debug shell device not specified!"
//    }
  }

  return result;
}

#endif  // CONFIG_USE_SEMIHOSTING
