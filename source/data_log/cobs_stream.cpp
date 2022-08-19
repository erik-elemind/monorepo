/*
 * cobs_stream.cpp
 *
 * Copyright (C) 2022 Elemind Technologies, Inc.
 *
 * Created: Apr, 2022
 * Author:  Paul Adelsbach
 */
#include "cobs_stream.h"

#include "data_log_internal.h"
#include "loglevels.h"
#include "config.h"

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

#if (defined(CONFIG_DATALOG_USE_COBSR_RLE0) && (CONFIG_DATALOG_USE_COBSR_RLE0 > 0U))
#include "COBSR_RLE0.h"
#else
// TODO: There are 2 "cobs.h" files in the project: interfaces/cobs.h
// and packet_serial/Encoding/COBS.h. This is including the
// packet_serial version in the current MCUXpresso project.
#include "cobs.h"
#endif

void cobs_stream_init(cobs_stream_ctx_t* p_ctx)
{
    p_ctx->workidx = 0;
}

void cobs_stream_decode(const uint8_t* buf, size_t bufsz, cobs_stream_ctx_t* p_ctx)
{
    uint8_t byte;

    for (uint32_t i=0; i<bufsz; i++) {
        // Read byte-at-a-time
        byte = buf[i];

        if (p_ctx->workidx < p_ctx->worksz) {
            // Accumulate bytes until we reach the marker
            p_ctx->work[p_ctx->workidx] = byte;

            if (byte == COBS_END_MARKER) {
#if (defined(CONFIG_DATALOG_USE_COBSR_RLE0) && (CONFIG_DATALOG_USE_COBSR_RLE0 > 0U))
                cobsr_decode_result cobs_result = cobsr_rle0_decode(
                    p_ctx->dest, p_ctx->destsz, p_ctx->work, p_ctx->workidx);
                if (cobs_result.status == COBSR_DECODE_OK) {
                    if (p_ctx && p_ctx->cb) {
                        p_ctx->cb(p_ctx->dest, cobs_result.out_len);
                    }
                }
                else {
                    LOGE("cobs_stream","cobs decode returned error %d\n", cobs_result.status);
                }
#else
                size_t decodesz = COBS::decode(p_ctx->work, p_ctx->workidx, p_ctx->dest);
                if (p_ctx && p_ctx->cb) {
                    p_ctx->cb(p_ctx->dest, decodesz);
                }
#endif

                // Reset the index into the working buffer.
                // Note we are dropping the COBS_END_MARKER byte.
                // Carry on through the loop in case to process more bytes
                // and possibly more complete messages.
                p_ctx->workidx = 0;
            }
            else {
                p_ctx->workidx++;
            }
        }

        else if (byte == COBS_END_MARKER) {
            // in overflow case, reset index upon receiving the end marker
            p_ctx->workidx = 0;
        }
    }
}

#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))
