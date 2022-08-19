#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>  // uint32_t, etc.

// Include FW_VERSION_[MAJOR|MINOR|STRING|INTEGER]
#include "version.h"

#include "config.h"


#ifndef TRUE
  #define TRUE 1
  #define FALSE 0
#endif

#ifndef MIN
  #define MIN(a,b) (((a)<(b))?(a):(b))
  #define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef ARRAY_SIZE
  #define ARRAY_SIZE(a)  (sizeof(a)/sizeof(a[0]))
#endif

#define SECONDS_TO_MS(sec) (sec * 1000)
#define MINUTES_TO_MS(min) (min * SECONDS_TO_MS(60))
#define HOURS_TO_MS(hours) (hours * MINUTES_TO_MS(60))

#ifdef __cplusplus
extern "C" {
#endif

void print_version(void);
#if defined(CONFIG_SHELL_USB) || defined(CONFIG_SHELL_UART)
void debug_uart_buffer_clear(void);
int get_recv_count(void);
void debug_uart_receive_pretask_init(void);
void debug_uart_recv_task(void *ignored);
int debug_uart_getc(int pos);
void debug_uart_puts(char* str);
void debug_uart_puts2(char* str, size_t len);
void debug_uart_puti(int i);
void debug_uart_puti_base(int i, int base);

// TODO: Reverse the naming convention between functions that and don't print newlines.
// the following functions do NOT automatically print newlines.
void debug_uart_puts_nnl(char* str);
void debug_uart_puti_nnl(int i);
void debug_uart_puti_base_nnl(int i, int base);

#else
// Do nothing if shell is UART
#define debug_uart_puts(X)
#define debug_uart_puts2(X,Y)
#define debug_uart_puti(X)
#define debug_uart_puti_base(X, Y)

#define debug_uart_puts_nnl(X)
#define debug_uart_puti_nnl(X)
#define debug_uart_puti_base_nnl(X, Y)

#endif

#ifdef __cplusplus
}
#endif

#endif  // UTILS_H
