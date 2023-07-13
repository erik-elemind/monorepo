#pragma once

#include <stdint.h>
#include <stddef.h> // for size_t

// This is the end of message delimter
#ifndef COBS_END_MARKER
#define COBS_END_MARKER (0)
#endif

// COBS streaming callback, passing the original destination buffer and 
// the decoded size.
typedef void (*cobs_stream_cb_t)(const uint8_t* buf, size_t bufsz);

typedef struct {
    uint8_t* dest;          // buffer to hold decoded data
    size_t   destsz;        // size of the buffer
    uint8_t* work;          // working buffer to accumulate received bytes
    size_t   worksz;        // size of working buffer
    uint32_t workidx;       // current index in the working buffer
    cobs_stream_cb_t cb;    // callback to invoke upon decode complete
} cobs_stream_ctx_t;

// Init the stream
void cobs_stream_init(cobs_stream_ctx_t* p_ctx);

// Process bytes or partial messages.
// Invokes the callback upon successful decode.
void cobs_stream_decode(const uint8_t* buf, size_t bufsz, cobs_stream_ctx_t* p_ctx);
