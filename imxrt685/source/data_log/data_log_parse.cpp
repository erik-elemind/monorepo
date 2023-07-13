/*
 * data_log_parse.c
 *
 * Copyright (C) 2022 Elemind Technologies, Inc.
 *
 * Created: Apr, 2022
 * Author:  Paul Adelsbach
 */
#include <stdbool.h>

#include "cobs_stream.h"
#include "data_log.h"
#include "data_log_internal.h"
#include "eeg_datatypes.h"
#include "ff.h"
#include "string_util.h"

#define LOG_LEVEL_MODULE    LOG_WARN
#include "loglevels.h"

#include "data_log_parse.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "heatshrink_decoder.h"

#ifdef __cplusplus
}
#endif

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

static const char *TAG = "data_log_parse";  // Logging prefix for this module

// Max encoded message size (in bytes)
#define MAX_COBS_MSG 256

// Output buffer must be at least as big as the max expected message (256)
static uint8_t g_cobsbuf[MAX_COBS_MSG];

// Work buffer must also be as big as the max encoded message.
static uint8_t g_cobsworkbuf[MAX_COBS_MSG];

// COBS decode context.
// Nordic SDK devs would make a macro for this, ie COBS_STREAM_DEF(mystream),
// but we can do it manually for now.
static cobs_stream_ctx_t g_cobsctx = {
    .dest = g_cobsbuf,
    .destsz = sizeof(g_cobsbuf),
    .work = g_cobsworkbuf,
    .worksz = sizeof(g_cobsworkbuf),
    .cb = NULL,
};

// Arbitrary sized working buffer for heatshrink decode.
// HEATSHRINK_STATIC_INPUT_BUFFER_SIZE is 32, so there may not be a
// big advantage of making this any larger.
static uint8_t g_hsd_buf[32];

// Heatshrink decode context
static heatshrink_decoder g_hsd;

// Stop requested flag
static bool g_parse_stop_flag = false;

// Status fields
// TODO: combine these and other globals into a common struct.
static size_t g_file_bytes;             // file size
static size_t g_file_bytes_processed;   // uncompressed bytes processed thus far

// Initialize the parser
static void data_log_parse_init(data_log_parse_cb_t packet_callback)
{
    heatshrink_decoder_reset(&g_hsd);
    g_cobsctx.cb = packet_callback;
    cobs_stream_init(&g_cobsctx);
    g_parse_stop_flag = false;
    g_file_bytes = 0;
    g_file_bytes_processed = 0;
}

// Parse a new chunk of the file
static int data_log_parse_chunk(uint8_t* buf, size_t bufsz)
{
    size_t count;
    int hsd_result;
    size_t bytes_sunk = 0;
    size_t bytes_polled = 0;

    while (bytes_sunk < bufsz) {
        // Send bytes through the decoder
        hsd_result = heatshrink_decoder_sink(&g_hsd, &buf[bytes_sunk], bufsz-bytes_sunk, &count);
        if (hsd_result != HSDR_SINK_OK) {
            LOGE(TAG, "hsd sink error: %d\n", hsd_result);
            return -1;
        }
        bytes_sunk += count;

        do {
            // Pull out the decoded bytes
            hsd_result = heatshrink_decoder_poll(&g_hsd, g_hsd_buf, sizeof(g_hsd_buf), &count);
            if (hsd_result != HSDR_POLL_EMPTY && hsd_result != HSDR_POLL_MORE) {
                LOGE(TAG, "hsd poll error: %d\n", hsd_result);
                return -1;
            }

            bytes_polled += count;
            LOGD(TAG, " bytes_polled=%u. hsd_result=%d\n", bytes_polled, hsd_result);

            // Push the new bytes through COBS.
            // This will invoke the COBS callback when an end marker is reached.
            cobs_stream_decode(g_hsd_buf, count, &g_cobsctx);
        } while (hsd_result == HSDR_POLL_MORE);
    }

    return 0;
}

// Finish any residual bytes left to process
static void data_log_parse_finish(void)
{
    size_t count;
    int hsd_result;

    // Finish off any remaining sunk bytes.
    // Docs say it may require more polling, though this is not observed in
    // initial testing.
    hsd_result = heatshrink_decoder_finish(&g_hsd);
    LOGD(TAG, " hsd complete. hsd_result=%d\n", hsd_result);
    while (hsd_result == HSDR_FINISH_MORE) {
        hsd_result = heatshrink_decoder_poll(&g_hsd, g_hsd_buf, sizeof(g_hsd_buf), &count);
        LOGD(TAG, " additional polling. hsd_result=%d, count=%u\n", hsd_result, count);

        // Send any new bytes through COBS as well
        cobs_stream_decode(g_hsd_buf, count, &g_cobsctx);

        // Attempt the finish again
        hsd_result = heatshrink_decoder_finish(&g_hsd);
    }
}


bool data_log_parse(const char* filename, data_log_parse_cb_t packet_callback)
{
    bool success = true;

    // Arbitrary sized buffer for reading in file chunks
    uint8_t buf[256];
    size_t bytes_read;

    char log_fname[MAX_PATH_LENGTH];
    size_t log_fsize = 0;
    log_fsize = str_append2(log_fname, log_fsize, DATA_LOG_DIR_PATH); // directory
    log_fsize = str_append2(log_fname, log_fsize, "/");               // path separator
    log_fsize = str_append2(log_fname, log_fsize, filename);          // log file name

    FIL file;
    FRESULT result = f_open(&file, log_fname, FA_READ);

    if (result == FR_OK) {
        // assume version 2 of the log file, so we need to discard the first 2 bytes
        f_read(&file, buf, 2, &bytes_read);

        data_log_parse_init(packet_callback);

        g_file_bytes = f_size(&file);

        while (1) {
            result = f_read(&file, buf, sizeof(buf), &bytes_read);
            if (result != FR_OK) {
              LOGE(TAG, "f_read() for %s returned %u\n", filename, result);
              success = false;
              break;
            }

            g_file_bytes_processed += bytes_read;

            if (bytes_read == 0) {
                // No bytes left
                break;
            }

            // Hacky stop flag implementation:
            // Break out of loop when stop has been requested.
            // TODO: Pass in callback that can check flag in main data_log context
            if (g_parse_stop_flag) {
                LOGE(TAG, "Stop flag set, aborting parse.\n");
                success = false;
                break;
            }

            int chunk_result = data_log_parse_chunk(buf, bytes_read);
            if (chunk_result < 0) {
                success = false;
                break;
            }
        }
        f_close(&file);

        data_log_parse_finish();
    }
    else {
        LOGE(TAG, "f_open() for %s returned %u\n", filename, result);
        success = false;
    }

    return success;
}

void data_log_parse_stop(void)
{
    // Set the flag to stop. The main processing loop checks this on each
    // chunk of data.
    g_parse_stop_flag = true;
}

void data_log_parse_stats(size_t* bytes_in_file, size_t* bytes_processed)
{
    *bytes_in_file = g_file_bytes;
    *bytes_processed = g_file_bytes_processed;
}

#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))
