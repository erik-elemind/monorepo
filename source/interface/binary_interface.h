#ifndef _BINARY_INTERFACE_H_
#define _BINARY_INTERFACE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "ErrVal.h"
#include "binary_reader.h"
#include "binary_writer.h"
#include "packet_serial.h"

#define BINARY_WRITE_BUFFER_SIZE BUFFER_SIZE

#define COMMAND_TYPE         uint8_t
#define COMMAND_ERRVAL_TYPE  ErrValUINT8
#define COMMAND_ERRVAL_READ  readUINT8
#define COMMAND_ERRVAL_WRITE writeUINT8
#define MAX_NUM_COMMANDS     (256U)


typedef void (*binitf_handler_func)(BinaryReader *r);

typedef struct Command
{
  COMMAND_TYPE code;
  binitf_handler_func binitf_handler_f;
} Command;

typedef struct BinaryInterface
{
  const Command *commands;
  size_t num_commands;
  PacketSerial pSerial;
  uint8_t bw_buffer[BINARY_WRITE_BUFFER_SIZE];
  BinaryReader br;
  BinaryWriter bw;
} BinaryInterface;

void bi_init(BinaryInterface *bi, const Command *commands, size_t num_commands);
bool handleMessagesPolling(BinaryInterface *bi);
bool handleMessages(BinaryInterface *bi, uint8_t data);
BinaryReader *getReader(BinaryInterface *bi);
BinaryWriter *getWriter(BinaryInterface *bi);
void writeCC(BinaryInterface *bi, uint8_t cc);

#endif // _BINARY_INTERFACE_H_
