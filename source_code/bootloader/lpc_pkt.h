/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: Low level packet interface for the LPC bootloader.
 *              See links to NXP documentation in lpc_pkt_defs.h.
 */

#pragma once

#include "lpc_pkt_defs.h"
#include <stdint.h>

typedef enum 
{
    LPC_PKT_EVT_TYPE_NONE,          // No event
    LPC_PKT_EVT_TYPE_RX,            // valid packet received
    LPC_PKT_EVT_TYPE_RX_ERROR_CRC,  // incorrect crc
    LPC_PKT_EVT_TYPE_RX_ERROR_LEN,  // data length exceeded
    LPC_PKT_EVT_TYPE_RX_ERROR_TYPE, // unknown or unsupported type
} lpc_pkt_evt_type_t;

typedef struct 
{
    lpc_pkt_evt_type_t  evt_type;   // event type
    pkt_t*              pkt;        // packet contents when evt_type is LPC_PKT_EVT_TYPE_RX
} lpc_pkt_evt_t;

/**@brief   Event handler function prototype.
 *
 * @param[in]   p_evt   The event.
 */
typedef void (*lpc_pkt_evt_handler_t)(const lpc_pkt_evt_t* p_evt);

/**@brief   Caller-provided function to send a buffer over uart.
 *
 * @param[in]   buf     Pointer to data buffer
 * @param[in]   len     Number of bytes in buffer to send
 * @retval 0 on success. non-zero on failure
 */
typedef int (*lpc_pkt_uart_tx_func_t)(const uint8_t* buf, uint32_t len);

/**@brief   Function for initializing the LPC packet interface.
 *
 * @param[in]   uart_tx_func    Pointer to UART transmit function.
 */
void lpc_pkt_init(lpc_pkt_uart_tx_func_t uart_tx_func);

/**@brief   Function for handling bytes received from the LPC.
 * 
 * Calls the handler upon full received packets, or error conditions.
 *
 * @param[in]   b       Byte received
 * 
 * @retval      Event detected, if any. NULL indicates no event. 
 *              Note: the memory pointed to by this return value
 *              is only valid until the next call to this function.
 */
const lpc_pkt_evt_t* lpc_pkt_handle_byte(uint8_t b);

/**@brief   Send a Ping packet over UART.
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_ping(void);

/**@brief   Send an Ack packet over UART.
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_ack(void);

/**@brief   Send a FlashEraseRegion packet over UART.
 * 
 * Erases all of LPC internal flash.
 * This procedure unlocks a secured device, and is needed to avoid NAKs 
 * responses for many other packets.
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_flash_erase_region1(void);
int lpc_send_flash_erase_region2(void);

/**@brief   Send a GetProperty packet over UART.
 *
 * @param[in]   prop    Property to request
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_get_property(property_type_t prop);

/**@brief   Send a Reset command packet over UART.
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_reset(void);

/**@brief   Send a WriteMemory packet over UART.
 *
 * @param[in]   len     Length in bytes
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_write_mem(uint32_t len);

/**@brief   Send a FillMemory packet over UART.
 *
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_fill_mem1(void);
int lpc_send_fill_mem2(void);

/**@brief   Send a ConfigureMemory packet over UART.
 *
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_config_mem(void);

/**@brief   Send a Data packet over UART.
 *
 * @param[in]   p_data  Pointer to data buffer
 * @param[in]   len     Length in bytes
 * 
 * @retval  0 on success. Non-zero on failure.
 */
int lpc_send_data(const uint8_t* p_data, uint32_t len);
