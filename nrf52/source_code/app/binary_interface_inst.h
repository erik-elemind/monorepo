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
    CC_UNUSED1 = 1,
    CC_UNUSED2 = 2,
    CC_NUS = 3,
    CC_CHARACTERISTIC = 4
}COMMAND_CODES;

void bin_itf_init();
bool bin_itf_handle_messages(uint8_t data);
bool bin_itf_send_command(char* buf, size_t size);
bool bin_itf_send_file_command(const uint8_t *buf, size_t buf_size);

#endif // _BINARY_INTERFACE_INST_H_
