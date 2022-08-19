/*
 * ble_uart_send.h
 *
 *  Created on: Apr 17, 2021
 *      Author: DavidWang
 */

#ifndef BLE_BLE_UART_SEND_H_
#define BLE_BLE_UART_SEND_H_

#include <stddef.h> // for size_t
#include <stdint.h> // for uint8_t

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

#include "ble.h" // for ELECTRODE_NUM
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(ENABLE_BLE_UART_SEND_TASK) && (ENABLE_BLE_UART_SEND_TASK > 0U))
// Init called before vTaskStartScheduler() launches our Task in main():
void ble_uart_send_pretask_init(void);

void ble_uart_send_task(void *ignored);
#endif // ENABLE_BLE_UART_SEND_TASK

int ble_uart_send_buf(const char* buf, size_t buf_size);

void ble_uart_send_battery_level(uint8_t battery_level);
void ble_uart_send_serial_number(char* serial_number);
void ble_uart_send_software_version(char* software_version);
void ble_uart_send_dfu();

void ble_uart_send_electrode_quality(uint8_t electrode_quality[ELECTRODE_NUM]);
void ble_uart_send_volume(uint8_t volume);
void ble_uart_send_power(uint8_t power);
void ble_uart_send_therapy(uint8_t therapy);
void ble_uart_send_heart_rate(uint8_t heart_rate);
void ble_uart_send_blink_status(uint8_t blink_status[BLINK_NUM]);
void ble_uart_send_quality_check(uint8_t quality_check);
void ble_uart_send_alarm(alarm_params_t *alarm);
void ble_uart_send_sound(uint8_t sound);
void ble_uart_send_time(uint64_t unix_epoch_time_sec);


#ifdef __cplusplus
}
#endif


#endif /* BLE_BLE_UART_SEND_H_ */
