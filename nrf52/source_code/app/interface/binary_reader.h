#ifndef _BINARY_PARSER_H_
#define _BINARY_PARSER_H_

#include "sdk_common.h"
#include "bit_copy.h"
#include "packet_serial.h"

#include <string.h>
#include <stdlib.h>

#include "bit_copy.h"

#include "ErrVal.h"
#include "ErrCodes.h"

#include "endian_util.h"
#include "InterfaceTypes.h"

#include <math.h> // used for ceil

/**********************************************************************/
// PARSE BINARY COMMAND STRUCT

typedef struct BinaryReader
{
  uint8_t *buffer_;
  size_t bitSize_;
  size_t bitIndex_;
  ENDIAN endian_;
} BinaryReader;

void br_reset(BinaryReader *br);
void br_init(BinaryReader *br, const uint8_t *buffer, const size_t byteSize);
bool read(BinaryReader *br, uint8_t *buf, const size_t bufBytes, const size_t numBits);

ErrValUINT readUINTX(BinaryReader *br, size_t bitSize);
ErrValUINT readUINT(BinaryReader *br);
ErrValUINT8 readUINT8(BinaryReader *br);
ErrValUINT16 readUINT16(BinaryReader *br);
ErrValUINT32 readUINT32(BinaryReader *br);
ErrValUINT64 readUINT64(BinaryReader *br);
ErrValINT readINTX(BinaryReader *br, size_t bitSize);
ErrValINT readINT(BinaryReader *br);
ErrValINT8 readINT8(BinaryReader *br);
ErrValINT16 readINT16(BinaryReader *br);
ErrValINT32 readINT32(BinaryReader *br);
ErrValINT64 readINT64(BinaryReader *br);
ErrValFLOAT readFLOAT(BinaryReader *br);
ErrValDOUBLE readDOUBLE(BinaryReader *br);
ErrValUINT8 readSTRING(BinaryReader *br, char *xbuf, int xsize);
ErrValBOOL readBOOLasByte(BinaryReader *br);
ErrValBOOL readBOOLasBit(BinaryReader *br);
uint8_t *getBuffer(BinaryReader *br);
size_t br_get_bit_index(BinaryReader *br);
size_t br_get_bit_size(BinaryReader *br);
size_t br_get_index(BinaryReader *br);
size_t br_get_size(BinaryReader *br);



#endif // _BINARY_PARSER_H_
