
#include "binary_writer.h"
#include "config.h"
#include "reentrant_math.h"

void bw_reset(BinaryWriter *bw)
{
    bw->bitIndex_ = 0;
}
void bw_init(BinaryWriter *bw, uint8_t *buffer, const size_t byteSize, PacketSerial *pSerial)
{
    bw->buffer_ = buffer;
    bw->bitCap_ = 8 * byteSize;
    bw->bitIndex_ = 0;
    bw->pSerial_ = pSerial;
}

bool write(BinaryWriter *bw, uint8_t *buf, const size_t bufBytes, const size_t numBits)
{
    if (bw->bitIndex_ + numBits <= bw->bitCap_)
    {
        // TODO: protect the buffer with an optional mutex, 
        //       provided/implemented by the caller.
        if (numBits == 0)
        {
            // do nothing
        }
        else if (bw->bitIndex_ % 8 == 0 && numBits % 8 == 0)
        {
            // if the bit boundaries are respected, use memcpy
            memcpy(bw->buffer_ + bw->bitIndex_ / 8, buf, numBits / 8); // dest, source, byte_size
        }
        else
        {
            copyBitsLittleEndian(bw->buffer_, buf, numBits, bw->bitIndex_, 0); // , dstNumBytes, bufBytes
        }
        bw->bitIndex_ += numBits;
        return true;
    }
    else
    {
        return false;
    }
}
bool writeUINT(BinaryWriter *bw, const unsigned int val, const size_t bitSize)
{
    return write(bw, (uint8_t *)&val, sizeof(val), bitSize);
}
bool writeUINT8(BinaryWriter *bw, const uint8_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 1 * 8);
}
bool writeUINT16(BinaryWriter *bw, const uint16_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 2 * 8);
}
bool writeUINT32(BinaryWriter *bw, const uint32_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 4 * 8);
}
bool writeUINT64(BinaryWriter *bw, const uint64_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 8 * 8);
}
bool writeINT(BinaryWriter *bw, const int val, const size_t bitSize)
{
    // Assuming "val" is stored as 2's compliment,
    // the write operation will truncate the sign extension.
    unsigned int uival = (unsigned int)val;
    return write(bw, (uint8_t *)&uival, sizeof(uival), bitSize);
}
bool writeINT8(BinaryWriter *bw, const int8_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 1 * 8);
}
bool writeINT16(BinaryWriter *bw, const int16_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 2 * 8);
}
bool writeINT32(BinaryWriter *bw, const int32_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 4 * 8);
}
bool writeINT64(BinaryWriter *bw, const int64_t val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 8 * 8);
}
bool writeFLOAT(BinaryWriter *bw, const float val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 4 * 8);
}
bool writeDOUBLE(BinaryWriter *bw, const double val)
{
    return write(bw, (uint8_t *)&val, sizeof(val), 8 * 8);
}
bool writeCSTRING(BinaryWriter *bw, char *str)
{
    return writeSTRING(bw, str, strlen(str));
}
bool writeSTRING(BinaryWriter *bw, char *str, const size_t str_size)
{
    size_t bitIndexBackup = bw->bitIndex_;
    // TODO: ensure these writes occur atomically
    if ((bw->bitIndex_ + 8 * (str_size + 1) <= bw->bitCap_) &&
        writeUINT8(bw, str_size) &&
        write(bw, (uint8_t *)str, str_size, str_size * 8))
    {
        return true;
    }
    else
    {
        bw->bitIndex_ = bitIndexBackup;
        return false;
    }
}

bool writeBOOLasByte(BinaryWriter *bw, const bool val)
{
    uint8_t numval = val ? 1 : 0;
    return writeUINT8(bw, numval);
}

bool writeBOOLasBit(BinaryWriter *bw, const bool val)
{
    uint8_t numval = val ? 1 : 0;
    // return write<unsigned int>(numval, 1);
    unsigned int uinumval = (unsigned int)numval;
    return write(bw, (uint8_t *)&uinumval, sizeof(uinumval), 1);
}

size_t bw_get_bit_size(BinaryWriter *bw)
{
    return bw->bitIndex_;
}

size_t bw_get_bit_capacity(BinaryWriter *bw)
{
    return bw->bitCap_;
}

size_t bw_get_size(BinaryWriter *bw)
{
  return rmath_divide_ceil(bw->bitIndex_ , 8);
}

size_t bw_get_capacity(BinaryWriter *bw)
{
    return bw->bitCap_ / 8;
}

bool bw_send(BinaryWriter *bw)
{
    // TODO: protect this operation with the mutex from above
    send(bw->pSerial_, bw->buffer_, bw_get_size(bw));
    // reset the buffer
    bw->bitIndex_ = 0;
    return true;
}


