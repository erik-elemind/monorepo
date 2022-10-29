#ifndef _BINARY_INTERFACE_INST_H_
#define _BINARY_INTERFACE_INST_H_

#include <stdint.h>
#include "binary_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: remove this
extern BinaryInterface bin_itf;

// TODO: move to binary_interface_inst.h
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
bool bin_itf_send_uart_command(char* buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif // _BINARY_INTERFACE_INST_H_
