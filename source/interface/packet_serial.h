// =============================================================================
// Copyright (c) 2013-2016 Christopher Baker <http://christopherbaker.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================

#ifndef _PACKET_SERIAL_H_
#define _PACKET_SERIAL_H_

#if 0
#include "sdk_common.h"
#else
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#include "cobs.h"
#include "slip.h"

typedef void (*packet_handler_func)(void* context, const uint8_t *buffer, size_t size);
typedef int (*serial_available_func)(void);
typedef uint8_t (*serial_read_func)(void);
typedef size_t (*serial_write_func)(uint8_t val);
typedef size_t (*serial_write_buffer_func)(const char *buffer, size_t size);
//typedef void (*serial_flush_func)(void);

typedef size_t (*encode_func)(const uint8_t *buffer, size_t size, uint8_t *encoded);
typedef size_t (*decode_func)(const uint8_t *buffer, size_t size, uint8_t *decoded);
typedef size_t (*get_encoded_buffer_size_func)(size_t sourceSize);

#define BUFFER_SIZE 256

typedef struct PacketSerial
{
    uint8_t recieve_buffer[BUFFER_SIZE];
    size_t recieve_buffer_index;
    uint8_t packet_marker;
    uint8_t decode_buffer[BUFFER_SIZE];
    uint8_t encode_buffer[BUFFER_SIZE];

    // packet handler
    void * on_packet_c; // context for the call to on_packet_f.
    packet_handler_func on_packet_f;

    // serial functions
    serial_available_func serial_available_f;
    serial_read_func serial_read_f;
    serial_write_func serial_write_f;
    serial_write_buffer_func serial_write_buffer_f;
//    serial_flush_func serial_flush_f;

    // encode and decode functions
    encode_func encode_f;
    decode_func decode_f;
    get_encoded_buffer_size_func get_encoded_buffer_size_f;
} PacketSerial;

void init_cobs_packet_serial(PacketSerial *ps, uint8_t packet_marker);
void init_slip_packet_serial(PacketSerial *ps, uint8_t packet_marker);
void update_polling(PacketSerial *ps);
void update(PacketSerial *ps, uint8_t data);
void send(PacketSerial *ps, const uint8_t *buffer, size_t size);

#endif //_PACKET_SERIAL_H_
