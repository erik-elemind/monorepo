#include "virtual_com.h"
#include "utils.h"
#include "reset_reason.h"
#include "prebuild.h"
#include "fw_version.h"
#include "zmodem.h"

//static const char *TAG = "utils";  // Logging prefix for this module
uint8_t rcv_byte[8192] = { '\0' };
int recv_cnt = 0;

void
print_version (void)
{
  printf(FW_VERSION_STRING);
  printf("\r\n");
}

void
print_version_full (void)
{
  printf(FW_VERSION_FULL);
  printf("\r\n");
}

#if defined(CONFIG_SHELL_USB)
void debug_uart_receive_pretask_init(void) {
  vQueueAddToRegistry(USART_DEBUG_RTOS_HANDLE.rxSemaphore, "debug_uart_rx_sem");
}

/* RTOS task implementation */
void debug_uart_recv_task(void *ignored) {
  uint8_t data[30] = "debug uart tx interrupt\r\n";
  size_t byte_count;

  USART_RTOS_Send(&USART_DEBUG_RTOS_HANDLE, &data[0], (uint32_t) strlen((const char*) data));
  while (1) {
    USART_RTOS_Receive(&USART_DEBUG_RTOS_HANDLE, &rcv_byte[recv_cnt++], 1, &byte_count);
    if (recv_cnt > sizeof(rcv_byte)) {
      recv_cnt = 0;
    }
  }
}

int get_recv_count() {
  return recv_cnt;
}

void debug_uart_buffer_clear(void) {
  memset(rcv_byte, 0x00, sizeof(rcv_byte));
  recv_cnt = 0;
}
int debug_uart_getc(int pos) {
  return rcv_byte[pos];
}
void debug_uart_puts(char* str)
{
  if(str == NULL){
    return;
  }
  size_t str_size = strlen(str);
  if(str_size==0){
    return;
  }
  USART_WriteBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t*)str,str_size);
  USART_WriteBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t*)"\n", 1);
}
void
debug_uart_puts_nnl(char* str)
{
  if(str == NULL){
    return;
  }
  size_t str_size = strlen(str);
  if(str_size==0){
    return;
  }
  USART_WriteBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t*)str,
      str_size);
}
#elif defined(CONFIG_SHELL_UART)
void
debug_uart_puts(char* str)
{
  virtual_com_write(str, strlen(str));
  virtual_com_write("\n", 1);
}
void
debug_uart_puts_nnl(char* str)
{
  virtual_com_write(str, strlen(str));
}
#endif

#if defined(CONFIG_SHELL_USB)
void
debug_uart_puts2(char* str, size_t len)
{
  if(len==0){
    return;
  }
  USART_WriteBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t*)str,
      len);
//  USART_WriteBlocking((&USART_DEBUG_RTOS_HANDLE)->base, (uint8_t*)"\n", 1);
}
#elif defined(CONFIG_SHELL_UART)
void
debug_uart_puts2(char* str, size_t len)
{
  virtual_com_write(str, len);
  virtual_com_write("\n", 1);
}
#endif

#if defined(CONFIG_SHELL_USB) || defined(CONFIG_SHELL_UART)
void
debug_uart_puti(int i)
{
  debug_uart_puti_base(i, 10);
}

void
debug_uart_puti_base(int i, int base)
{
  char buf[33]; // max length of string in base 2
  switch (base){
    case 8:   snprintf ( buf, 33, "%o", i ); break;
    case 10:  snprintf ( buf, 33, "%d", i ); break;
    case 16:  snprintf ( buf, 33, "%x", i ); break;
    default:  snprintf ( buf, 33, "%d", i ); break;
  }
  debug_uart_puts(buf);
}

void
debug_uart_puti_nnl(int i)
{
  debug_uart_puti_base_nnl(i, 10);
}

void
debug_uart_puti_base_nnl(int i, int base){
  char buf[33]; // max length of string in base 2
  switch (base){
    case 8:   snprintf ( buf, 33, "%o", i ); break;
    case 10:  snprintf ( buf, 33, "%d", i ); break;
    case 16:  snprintf ( buf, 33, "%x", i ); break;
    default:  snprintf ( buf, 33, "%d", i ); break;
  }
  debug_uart_puts_nnl(buf);
}

#endif
