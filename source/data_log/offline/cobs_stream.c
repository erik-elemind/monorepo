#include "cobs_stream.h"
#include "loglevels.h"

#ifdef COBS_MODE_PLAIN
// Example data log files use this.
// Use this for offline testing only.
#include "cobs.h"
#elif defined(COBS_MODE_R)
// not supported
#else // COBS_MODE_RLE0
// Current data logs use this version.
#include "COBSR_RLE0.h"
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
                #ifdef COBS_MODE_PLAIN
                size_t decodesz = cobs_decode(p_ctx->work, p_ctx->workidx, p_ctx->dest);
                if (p_ctx && p_ctx->cb) {
                    p_ctx->cb(p_ctx->dest, decodesz);
                }
                #else
                cobsr_decode_result cobs_result = cobsr_rle0_decode(
                    p_ctx->dest, p_ctx->destsz, p_ctx->work, p_ctx->workidx);
                if (cobs_result.status == COBSR_DECODE_OK) {
                    if (p_ctx && p_ctx->cb) {
                        p_ctx->cb(p_ctx->dest, cobs_result.out_len);
                    }
                }
                else {
                    LOGE("cobs decode returned error %d\n", cobs_result.status);
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
