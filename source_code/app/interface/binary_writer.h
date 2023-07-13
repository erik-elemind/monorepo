#ifndef _BINARY_WRITER_H_
#define _BINARY_WRITER_H_

#include "sdk_common.h"
#include "bit_copy.h"
#include "packet_serial.h"
#include <math.h> // used for ceil

typedef struct BinaryWriter
{
  uint8_t *buffer_;
  size_t bitCap_;
  size_t bitIndex_;
  PacketSerial *pSerial_;
} BinaryWriter;

void bw_reset(BinaryWriter *bw);
void bw_init(BinaryWriter *bw, uint8_t *buffer, const size_t byteSize, PacketSerial *pSerial);
bool write(BinaryWriter *bw, uint8_t *buf, const size_t bufBytes, const size_t numBits);
bool writeUINT(BinaryWriter *bw, const unsigned int val, const size_t bitSize);
bool writeUINT8(BinaryWriter *bw, const uint8_t val);
bool writeUINT16(BinaryWriter *bw, const uint16_t val);
bool writeUINT32(BinaryWriter *bw, const uint32_t val);
bool writeUINT64(BinaryWriter *bw, const uint64_t val);
bool writeINT(BinaryWriter *bw, const int val, const size_t bitSize);
bool writeINT8(BinaryWriter *bw, const int8_t val);
bool writeINT16(BinaryWriter *bw, const int16_t val);
bool writeINT32(BinaryWriter *bw, const int32_t val);
bool writeINT64(BinaryWriter *bw, const int64_t val);
bool writeFLOAT(BinaryWriter *bw, const float val);
bool writeDOUBLE(BinaryWriter *bw, const double val);
bool writeCSTRING(BinaryWriter *bw, char *str);
bool writeSTRING(BinaryWriter *bw, char *str, const size_t str_size);
bool writeBOOLasByte(BinaryWriter *bw, const bool val);
bool writeBOOLasBit(BinaryWriter *bw, const bool val);
size_t bw_get_bit_size(BinaryWriter *bw);
size_t bw_get_bit_capacity(BinaryWriter *bw);
size_t bw_get_size(BinaryWriter *bw);
size_t bw_get_capacity(BinaryWriter *bw);
bool bw_send(BinaryWriter *bw);

#endif // _BINARY_WRITER_H_
