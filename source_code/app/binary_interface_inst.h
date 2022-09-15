#ifndef _BINARY_INTERFACE_INST_H_
#define _BINARY_INTERFACE_INST_H_

#include "binary_interface.h"
#include "lpc_uart.h"
#include "packet_serial.h"
#include "nrf_log.h"
#include "app_uart.h"

extern BinaryInterface bin_itf;

typedef enum COMMAND_CODES
{
    CC_NONE = 0,
    CC_SHKREQ = 1,
    CC_SHKRSP = 2,
    CC_FILE = 3,
    CC_COMMANDS = 4,
    CC_ACK = 5
}COMMAND_CODES;

void bin_itf_init();
bool bin_itf_handle_messages(uint8_t data);
bool bin_itf_send_command(char* buf, size_t size);
bool bin_itf_send_file_command(const uint8_t *buf, size_t buf_size);
bool bin_itf_send_ack();

#endif // _BINARY_INTERFACE_INST_H_
