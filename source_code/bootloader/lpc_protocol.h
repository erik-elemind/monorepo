/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: LPC interface for applying FW updates.
 */

#pragma once

#include <stdint.h>

/**@brief   Init the LPC interface protocol, and perform the autobaud sync 
 *          procedure. 
 * 
 * NOTE: LPC must be placed in ISP mode prior to this call.
 *
 * @retval  0 on success, nonzero on failure
 */
int lpc_protocol_init(void);

/**@brief   Apply the FW update to the LPC.
 * 
 * @param[in]   file_sz     Number of bytes in file
 *
 * @retval  0 on success, nonzero on failure
 */
int lpc_protocol_apply_fw(uint32_t file_sz);

/**@brief   Test routine which reads several properties from the LPC.
 *
 * @retval  0 on success, nonzero on failure
 */
int lpc_property_read(void);

/**@brief   Caller-provided function called by the LPC code to read the next 
 *          chunk of the FW file. 
 * 
 * NOTE: Caller must implement this routine.
 * 
 * @param[in]   offset  Offset in the file to read
 * @param[in]   p_buf   Buffer to where the bytes should be copied.
 * @param[in]   len     Number of bytes requested.
 *
 * @retval  0 on success, nonzero on failure
 */
int lpc_fw_file_read(uint32_t offset, uint8_t* p_buf, uint32_t len);

/**@brief   Caller-provided function to read a byte from uart.
 *
 * @param[in]   byte    Pointer to location to store the byte
 * 
 * @retval 0 on success. non-zero means no bytes available
 */
int lpc_uart_read_byte(uint8_t* byte);

/**@brief   Caller-provided function to send a buffer over uart.
 *
 * @param[in]   buf     Pointer to data buffer
 * @param[in]   len     Number of bytes in buffer to send
 * @retval 0 on success. non-zero on failure
 */
int lpc_uart_write(const uint8_t* buf, uint32_t len);
