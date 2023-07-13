
#include "binary_reader.h"
#include <stdio.h>

void br_reset(BinaryReader *br)
{
  br->bitIndex_ = 0;
}

void br_init(BinaryReader *br, const uint8_t *buffer, const size_t byteSize)
{
  br->buffer_ = (uint8_t *)buffer;
  br->bitSize_ = 8 * byteSize;
  br->bitIndex_ = 0;
  br->endian_ = getEndian();
}

// TODO: Endian not implemented.
static bool readInternal(BinaryReader *br, uint8_t *buf, const size_t bufBytes, const size_t numBits, ENDIAN endian)
{
//  printf("%d <= %d", (br->bitIndex_ + numBits), (br->bitSize_));
  if (br->bitIndex_ + numBits <= br->bitSize_)
  {
    if (numBits == 0)
    {
      // do nothing
    }
    else
    {
      if (br->bitIndex_ % 8 == 0 && numBits % 8 == 0)
      {
        // if the bit boundaries are respected, use memcpy
        memcpy(buf, br->buffer_ + br->bitIndex_ / 8, numBits / 8); // dest, source, byte_size
      }
      else
      {
        copyBitsLittleEndian(buf, br->buffer_, numBits, 0, br->bitIndex_); // bufBytes, srcNumBytes
      }
      br->bitIndex_ += numBits;
    }
    return true;
  }
  else
  {
    return false;
  }
}

bool read(BinaryReader *br, uint8_t *buf, const size_t bufBytes, const size_t numBits)
{
  return readInternal(br, buf, bufBytes, numBits, br->endian_);
}

ErrValUINT readUINTX(BinaryReader *br, size_t bitSize)
{
  ErrValUINT errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(unsigned int), bitSize))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValUINT readUINT(BinaryReader *br)
{
  return readUINTX(br, DEFAULT_UINT_BITS);
}
ErrValUINT8 readUINT8(BinaryReader *br)
{
  ErrValUINT8 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(uint8_t), 1 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValUINT16 readUINT16(BinaryReader *br)
{
  ErrValUINT16 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(uint16_t), 2 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValUINT32 readUINT32(BinaryReader *br)
{
  ErrValUINT32 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(uint32_t), 4 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValUINT64 readUINT64(BinaryReader *br)
{
  ErrValUINT64 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(uint64_t), 8 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValINT readINTX(BinaryReader *br, size_t bitSize)
{
  ErrValUINT result = readUINTX(br, bitSize);
  if (result.error_ != ERROR_NONE)
  {
    ErrValINT errval = {.error_ = result.error_, .value_ = 0};
    return errval;
  }
  else
  {
    unsigned int ui = result.value_;
    if (((ui >> (bitSize - 1)) & 1U) == 0)
    {
      // positive number
      ErrValINT errval = {.error_ = ERROR_NONE, .value_ = ui};
      return errval;
    }
    else
    {
      // negative number, convert from 2s compliment
      ui = (~ui & BITMASKU8(bitSize)) + 1;
      ErrValINT errval = {.error_ = ERROR_NONE, .value_ = -(int)ui};
      return errval;
    }
  }
}
ErrValINT readINT(BinaryReader *br)
{
  return readINTX(br, DEFAULT_INT_BITS);
}
ErrValINT8 readINT8(BinaryReader *br)
{
  ErrValINT8 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(int8_t), 1 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValINT16 readINT16(BinaryReader *br)
{
  ErrValINT16 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(int16_t), 2 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValINT32 readINT32(BinaryReader *br)
{
  ErrValINT32 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(int32_t), 4 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValINT64 readINT64(BinaryReader *br)
{
  ErrValINT64 errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(int64_t), 8 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}

ErrValFLOAT readFLOAT(BinaryReader *br)
{
  ErrValFLOAT errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(float), 4 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValDOUBLE readDOUBLE(BinaryReader *br)
{
  ErrValDOUBLE errval = {.error_ = ERROR_NONE, .value_ = 0};
  if (!read(br, (uint8_t *)&(errval.value_), sizeof(float), 8 * 8))
  {
    errval.error_ = ERROR_INDEX_OUT_OF_BOUNDS;
  }
  return errval;
}
ErrValUINT8 readSTRING(BinaryReader *br, char *xbuf, int xsize)
{
  ErrValUINT8 errval_idx_error = {.error_ = ERROR_INDEX_OUT_OF_BOUNDS, .value_ = 0};
  ErrValUINT8 result = readUINT8(br);
  if (result.error_ != ERROR_NONE)
  {
    return errval_idx_error;
  }
  uint8_t str_size = result.value_;
  if (str_size < xsize)
  {
    if (read(br, (uint8_t *)xbuf, str_size, str_size * 8))
    {
      xbuf[str_size] = '\0';
      result.value_ = str_size;
      return result;
    }
    else
    {
      br->bitIndex_ -= 8; // undo reading the bits from the call to "readUINT8".
      return errval_idx_error;
    }
  }
  else
  {
    if (read(br, (uint8_t *)xbuf, xsize - 1, (xsize - 1) * 8))
    {
      xbuf[xsize - 1] = '\0';
      result.value_ = xsize - 1;
      return result;
    }
    else
    {
      br->bitIndex_ -= 8; // undo reading the bits from the call to "readUINT8".
      return errval_idx_error;
    }
  }
}
ErrValBOOL readBOOLasByte(BinaryReader *br)
{
  uint8_t numval = 0;
  ErrValUINT8 result = readUINT8(br);
  if (result.error_ != ERROR_NONE)
  {
    ErrValBOOL errval = {.error_ = result.error_, .value_ = false};
    return errval;
  }
  numval = result.value_;
  ErrValBOOL errval = {.error_=ERROR_NONE, .value_= (numval != 0) };
  return errval;
}

ErrValBOOL readBOOLasBit(BinaryReader *br)
{
  uint8_t numval = 0;
  ErrValUINT result = readUINTX(br, 1);
  if (result.error_ != ERROR_NONE)
  {
    ErrValBOOL errval = {.error_ = result.error_, .value_ = false};
    return errval;
  }
  numval = result.value_;
  ErrValBOOL errval = {.error_=ERROR_NONE, .value_= (numval != 0) };
  return errval;
}

uint8_t *getBuffer(BinaryReader *br)
{
  return br->buffer_;
}
size_t br_get_bit_index(BinaryReader *br)
{
  return br->bitIndex_;
}
size_t br_get_bit_size(BinaryReader *br)
{
  return br->bitSize_;
}
size_t br_get_index(BinaryReader *br)
{
  return ceil(br->bitIndex_ / 8);
}
size_t br_get_size(BinaryReader *br)
{
  return ceil(br->bitSize_ / 8);
}
