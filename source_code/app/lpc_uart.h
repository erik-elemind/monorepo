/*
 * lpc_uart.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: LPC UART interface for Elemind Morpheus.
 *
 */
#ifndef __LPC_UART_H__
#define __LPC_UART_H__

#include <stdint.h>
#include <stdbool.h>

#include "ble_bas.h"

#include "ble.h"
#include "ble_elemind.h"


typedef union
{
  struct {
    // lsb
    bool sat : 1;
    bool fri : 1;
    bool thu : 1;
    bool wed : 1;
    bool tue : 1;
    bool mon : 1;
    bool sun : 1;
    bool on  : 1;
    uint32_t minutes_after_midnight : 16;
    // msb
  };
  uint8_t all[3];
} alarm_params_t;


/** Maximum command/response line length (including parameters).
  Must be < 255 (buffer length stored as uint8_t). */
#define LPC_MAX_COMMAND_SIZE 64


/** Initialize the LPC UART interface.

    @return NRF_SUCCESS on successful initialization,
    otherwise an error code.
*/
ret_code_t
lpc_uart_init(ble_bas_t* p_bas, ble_elemind_t* p_elemind);


/** Send command or request to LPC.

    @param[in] p_line Data to send to LPC (without newline)

    @return NRF_SUCCESS if transmission successful, otherwise an error
    code.
*/
ret_code_t
lpc_uart_sendline(const char* p_line);

/** Send command with byte buffer parameter to LPC.

    @param[in] buf Buffer to send
    @param[in] buf_size buf size, in bytes

    @return NRF_SUCCESS if transmission successful, otherwise an error
    code.
*/
ret_code_t
lpc_uart_send_buffer(uint8_t* buf, size_t buf_size);

/** Handle a command from the LPC UART.

    @param[in] p_command Command received
    @param[in] length Command length, in bytes

    @return NRF_SUCCESS if parsing successful, otherwise an error
    code.
*/
void
lpc_uart_parse_command(char* p_command, uint8_t length);

#endif /* __LPC_UART_H__ */
