/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: UART interface for tests running on a PC/laptop host.
 */

#pragma once

#include <stdint.h>

/**@brief   Init a uart port
 *
 * @param[in]   port        Path to UART port. Eg. "/dev/cu.usbmodem.1234"
 * @param[in]   settings    Port settings for bits, parity, and stop bits. Eg. "8N1"
 * @param[in]   baudrate    Baud rate
 *
 * @retval  0 on success, nonzero on failure
 */
int uart_init(const char* port, const char settings[3], const uint32_t baudrate);

/**@brief   Read a byte from the UART port, in non-blocking fashion.
 *
 * @param[out]  byte        Byte that was read.
 *
 * @retval  0 on success, nonzero on failure
 */
int uart_read_byte(uint8_t* byte);

/**@brief   Write a buffer to the UART port.
 *
 * @param[in]   buf     Pointer to buffer
 * @param[in]   len     Number of bytes to send
 *
 * @retval  0 on success, nonzero on failure
 */
int uart_write(const uint8_t* buf, uint32_t len);
