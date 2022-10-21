/*
 * ble_uart_send.c
 *
 *  Created on: Apr 17, 2021
 *      Author: DavidWang
 */

#include <ble_uart_send.h>
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "loglevels.h"
#include "string_util.h"
#include "message_buffer.h"

#include "FreeRTOS.h"
#include "task.h"

#include "binary_interface_inst.h"

/** Maximum length of response, in bytes. */
#define MAX_RESPONSE_SIZE 254

#if (defined(ENABLE_BLE_UART_SEND_TASK) && (ENABLE_BLE_UART_SEND_TASK > 0U))

#define BLE_UART_SEND_BUFFER_SIZE (1024)

static const char *TAG = "ble_uart_send"; // Logging prefix for this module

static uint8_t g_sbuf_array[BLE_UART_SEND_BUFFER_SIZE];
static StaticStreamBuffer_t g_sbuf_struct;
static StreamBufferHandle_t g_sbuf_handle;

int
ble_uart_send_buf(const char* buf, size_t buf_size)
{
  return xStreamBufferSend(g_sbuf_handle, buf, buf_size, portMAX_DELAY);
}

void ble_uart_send_pretask_init(void){
  // Any pre-scheduler init goes here.
  // Create the BLE message buffer
  g_sbuf_handle = xStreamBufferCreateStatic(
      sizeof(g_sbuf_array),
      1,
      g_sbuf_array,
      &(g_sbuf_struct) );


  // Add USART TX semaphore to registry
  vQueueAddToRegistry(USART_BLE_RTOS_HANDLE.txSemaphore, "ble_uart_tx_sem");
}

static void
task_init()
{
  // Any post-scheduler init goes here.
  LOGV(TAG, "Task launched. Entering event loop.\n\r");
}

/* RTOS task implementation */
void
ble_uart_send_task(void *ignored)
{
  static uint8_t g_write_buffer[MAX_RESPONSE_SIZE];

  task_init();

  while (1) {

    size_t byte_count = xStreamBufferReceive(g_sbuf_handle, g_write_buffer,
          sizeof(g_write_buffer), 0);

    if (byte_count != 0) {
      USART_RTOS_Send(&USART_BLE_RTOS_HANDLE, g_write_buffer, byte_count);
    }

  }
}


#else // defined(ENABLE_BLE_UART_SEND_TASK)

int
ble_uart_send_buf(const char* buf, size_t buf_size)
{
//  debug_uart_puts2((char*)buf,buf_size); // TODO: Delete

//  char msg[20];
//  snprintf(msg, 20, "S: [%d-%d,%d] #%d", buf[0], buf[buf_size-2], buf[buf_size-1], (int)buf_size);
//  debug_uart_puts(msg);

//  LOGV("ble_uart_send","buf_size: %u", buf_size);

  return USART_RTOS_Send(&USART_BLE_RTOS_HANDLE, (uint8_t*) buf, buf_size);
}


#endif // defined(ENABLE_BLE_UART_SEND_TASK)



// TODO: the following functions operate at a higher layer and 
//       should be moved to their own file.


static void
ble_uart_send_uint8_value(char* name, uint8_t value)
{
  char response[MAX_RESPONSE_SIZE] = { 0 };
  snprintf(response, sizeof(response)-1, "%s %d\r\n", name, value);

#if 0
  char buffer[256];
  size_t buffer_size = readable_cstr(buffer, 256, (char*) response);
  debug_uart_puts("ble_uart_send_uint8_value");
  debug_uart_puts2(buffer, buffer_size);
  debug_uart_puti(strlen(response));
#endif

  bin_itf_send_command(response, strlen(response));
}

static void
ble_uart_send_string_value(char* name, char* value)
{
  char response[MAX_RESPONSE_SIZE] = { 0 };
  snprintf(response, sizeof(response)-1, "%s %s\r\n", name, value);

  bin_itf_send_command(response, strlen(response));
}

void
ble_uart_send_battery_level(uint8_t battery_level)
{
  ble_uart_send_uint8_value("ble_battery_level", battery_level);
}

void
ble_uart_send_serial_number(char* serial_number)
{
  ble_uart_send_string_value("ble_serial_number", serial_number);
}

void
ble_uart_send_software_version(char* software_version)
{
  ble_uart_send_string_value("ble_software_version", software_version);
}

void
ble_uart_send_dfu()
{
  // Sends command with whitespace at end, but that's OK--parser handles it.
  ble_uart_send_string_value("ble_dfu", "");
}

void
ble_uart_send_electrode_quality(uint8_t electrode_quality[ELECTRODE_NUM])
{
  char response[MAX_RESPONSE_SIZE] = { 0 };
  snprintf(response, sizeof(response)-1,
    "ble_electrode_quality %d %d %d %d %d %d %d %d\r\n",
    electrode_quality[0], electrode_quality[1], electrode_quality[2],
    electrode_quality[3], electrode_quality[4], electrode_quality[5],
    electrode_quality[6], electrode_quality[7]);

  bin_itf_send_command(response, strlen(response));
}

void
ble_uart_send_volume(uint8_t volume)
{
  ble_uart_send_uint8_value("ble_volume", volume);
}

void
ble_uart_send_power(uint8_t power)
{
  ble_uart_send_uint8_value("ble_power", power);
}

void
ble_uart_send_therapy(uint8_t therapy)
{
  ble_uart_send_uint8_value("ble_therapy", therapy);
}

void
ble_uart_send_heart_rate(uint8_t heart_rate)
{
  ble_uart_send_uint8_value("ble_heart_rate", heart_rate);
}

void
ble_uart_send_blink_status(uint8_t blink_status[BLINK_NUM]){
  char response[MAX_RESPONSE_SIZE] = { 0 };
  snprintf(response, sizeof(response)-1,
    "ble_blink_status %d %d %d %d %d %d %d %d %d %d\r\n",
    blink_status[0], blink_status[1], blink_status[2],
    blink_status[3], blink_status[4], blink_status[5],
    blink_status[6], blink_status[7], blink_status[8],
    blink_status[9]);

  bin_itf_send_command(response, strlen(response));
}

void
ble_uart_send_quality_check(uint8_t quality_check)
{
  ble_uart_send_uint8_value("ble_quality_check", quality_check);
}

void ble_uart_send_alarm(alarm_params_t *alarm){
  char response[MAX_RESPONSE_SIZE] = { 0 };
  snprintf(response, sizeof(response)-1,
    "ble_alarm %d %d %d %d %d %d %d %d %d\r\n",
    alarm->flags.on, 
    alarm->flags.sun, 
    alarm->flags.mon, 
    alarm->flags.tue, 
    alarm->flags.wed, 
    alarm->flags.thu, 
    alarm->flags.fri, 
    alarm->flags.sat, 
    alarm->minutes_after_midnight);
  LOGV("ble_uart_send","%s",response);
  bin_itf_send_command(response, strlen(response));
}

void ble_uart_send_sound(uint8_t sound){
  ble_uart_send_uint8_value("ble_sound", sound);
}

void ble_uart_send_time(uint64_t unix_epoch_time_sec){
  char response[MAX_RESPONSE_SIZE] = { 0 };
  snprintf(response, sizeof(response)-1,
    "ble_time %lu\r\n", // TODO: Add support for uint64_t, llu types in printf
    (uint32_t)unix_epoch_time_sec); // (unsigned int)

  bin_itf_send_command(response, strlen(response));
}












