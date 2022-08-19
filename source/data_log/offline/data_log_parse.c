#include "heatshrink_decoder.h"
#include "cobs_stream.h"
#include "data_log.h"
#include "eeg_datatypes.h"
#include "loglevels.h"

#include <stdbool.h>

// Forward declaration
static void cobs_decode_cb(const uint8_t* buf, size_t bufsz);

// COBS decode context.
// Nordic SDK devs would make a macro for this, ie COBS_STREAM_DEF(mystream),
// but we can do it manually for now.
// Output buffer must be at least as big as the max expected message (256)
static uint8_t cobsbuf[256];
// Work buffer must also be as big as the max encoded message.
static uint8_t cobsworkbuf[256];
static cobs_stream_ctx_t cobsctx = {
    .dest = cobsbuf,
    .destsz = sizeof(cobsbuf),
    .work = cobsworkbuf,
    .worksz = sizeof(cobsworkbuf),
    .cb = cobs_decode_cb,
};

// Arbitrary sized working buffer for heatshrink decode.
// HEATSHRINK_STATIC_INPUT_BUFFER_SIZE is 32, so there may not be a 
// big advantage of making this any larger.
static uint8_t hsd_buf[32];
// Heatshrink decode context
static heatshrink_decoder hsd;

// Stop requested flag
static bool g_parse_stop_flag = false;

// Returns the enum as a string. Helpful for logs.
static const char* data_log_packet_str(data_log_packet_t type)
{
    switch (type)
    {
        case DLPT_EEG_START: return "DLPT_EEG_START";
        case DLPT_EEG_DATA: return "DLPT_EEG_DATA";
        case DLPT_INST_AMP_PHS: return "DLPT_INST_AMP_PHS";
        case DLPT_PULSE_START_STOP: return "DLPT_PULSE_START_STOP";
        case DLPT_EEG_DATA_PACKED: return "DLPT_EEG_DATA_PACKED";
        case DLPT_INST_AMP_PHS_PACKED: return "DLPT_INST_AMP_PHS_PACKED";
        case DLPT_ECHT_CHANNEL: return "DLPT_ECHT_CHANNEL";
        case DLPT_SWITCH: return "DLPT_SWITCH";
        case DLPT_STIM_AMP: return "DLPT_STIM_AMP";
        case DLPT_STIM_AMP_PACKED: return "DLPT_STIM_AMP_PACKED";
        case DLPT_CMD: return "DLPT_CMD";
        case DLPT_ACCEL_XYZ: return "DLPT_ACCEL_XYZ";
        case DLPT_ACCEL_TEMP: return "DLPT_ACCEL_TEMP";
        case DLPT_ALS: return "DLPT_ALS";
        case DLPT_MIC: return "DLPT_MIC";
        case DLPT_TEMP: return "DLPT_TEMP";
        case DLPT_EEG_COMP_HEADER: return "DLPT_EEG_COMP_HEADER";
        case DLPT_EEG_COMP_FRAME: return "DLPT_EEG_COMP_FRAME";
    }

    return "unkonwn";
}

// Callback when a COBS block is decoded successfully
static void cobs_decode_cb(const uint8_t* buf, size_t bufsz)
{
    LOGD("cobs cb. size=%zu, byte[0]=0x%02x (%u) byte[1]=0x%02x (%d)\n", 
        bufsz, buf[0], buf[0], buf[1], buf[1]);

    // First byte is the packet marker (aka type field):
    data_log_packet_t type = buf[0];
    LOGI("cobs decode cb. type=%d (%s), len=%zu\n", type, data_log_packet_str(type), bufsz);

    if (type == DLPT_EEG_DATA_PACKED) {
        uint32_t idx = 1; // start at the sample num offset
        uint32_t sample_num = *(uint32_t*)&buf[idx];
        idx += sizeof(sample_num);
        int32_t eeg_sample[MAX_NUM_EEG_CHANNELS];
        LOGI("  sample_num=%u\n", sample_num);

        // Actual samples start at index 5
        // Pull out the 3 EEG channels for each of the samples
        for (uint32_t i=0; i<EEG_PACK_NUM_SAMPLES_TO_SEND; i++) {
            for (uint32_t j=0; j<MAX_NUM_EEG_CHANNELS; j++, idx+=3) {
                // Each channel is encoded as 3 bytes
                // See this line in data_log_eeg():
                // add_to_buffer(g_eeg_pack_buffer, EEG_PACK_BUFFER_SIZE, g_eeg_pack_offset, eeg_channel_as_bytes, 3);
                // Little endian encode (I think)
                eeg_sample[j] = (buf[idx+2]<<16) + (buf[idx+1]<<8) + buf[idx];

                // Sign extend from 24b to 32b
                if (eeg_sample[j] & 0x800000) {
                    eeg_sample[j] |= 0xFF000000;
                }
            }

            LOGI("  samples=[%d,%d,%d]\n", 
                eeg_sample[0], eeg_sample[1], eeg_sample[2]
            );

            // TODO_COMPRESSION:
            // Call compress_frame() on the data and save bytes to the file.
            // Then COBS encode and heatshrink encode
            // See handle_comp_eeg() for the runtime implemenation as a reference.
        }
        // Just sanity checking the index to make sure we didn't exceed the len
        LOGI("  idx=%d\n", idx);
    }
    else {
        // TODO_COMPRESSION:
        // This is a non-EEG sample which does not need further processing.
        // This block should be saved back uncompressed and unmodified.
        // However, we need to re-encode with COBS, then pipe through heatshrink encode.
        // While it could be possible to retrieve the original COBS-encoded block,
        // it would still need to be re-heatshrinked.
        // See data_log_write() as a reference.
    }
}

// Initialize the parser
static void data_log_parse_init(void)
{
    heatshrink_decoder_reset(&hsd);
    cobs_stream_init(&cobsctx);
    g_parse_stop_flag = false;
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
        hsd_result = heatshrink_decoder_sink(&hsd, &buf[bytes_sunk], bufsz-bytes_sunk, &count);
        if (hsd_result != HSDR_SINK_OK) {
            LOGE("hsd sink error: %d\n", hsd_result);
            return -1;
        }
        bytes_sunk += count;

        do {
            // Pull out the decoded bytes
            hsd_result = heatshrink_decoder_poll(&hsd, hsd_buf, sizeof(hsd_buf), &count);
            if (hsd_result != HSDR_POLL_EMPTY && hsd_result != HSDR_POLL_MORE) {
                LOGE("hsd poll error: %d\n", hsd_result);
                return -1;
            }

            bytes_polled += count;
            LOGD(" bytes_polled=%zu. hsd_result=%d\n", bytes_polled, hsd_result);

            // Push the new bytes through COBS.
            // This will invoke the COBS callback when an end marker is reached.
            cobs_stream_decode(hsd_buf, count, &cobsctx);
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
    hsd_result = heatshrink_decoder_finish(&hsd);
    LOGD(" hsd complete. hsd_result=%d\n", hsd_result);
    while (hsd_result == HSDR_FINISH_MORE) {
        hsd_result = heatshrink_decoder_poll(&hsd, hsd_buf, sizeof(hsd_buf), &count);
        LOGD(" additional polling. hsd_result=%d, count=%zu\n", hsd_result, count);

        // Send any new bytes through COBS as well
        cobs_stream_decode(hsd_buf, count, &cobsctx);
        // Attemp the finish again
        hsd_result = heatshrink_decoder_finish(&hsd);
    }
}

#ifdef DL_PARSER_OFFLINE
void data_log_parse(const char* fn)
{
    // Arbitrary sized buffer for reading in file chunks
    uint8_t buf[256];
    size_t bytes_read;

    FILE* file = fopen(fn, "r");
    if (file) {
        data_log_parse_init();

        while (1) {
            bytes_read = fread(buf, 1, sizeof(buf), file);
            if (bytes_read == 0) {
                // No bytes left
                break;
            }

            // Hacky stop flag implementation:
            // Break out of loop when stop has been requested.
            if (g_parse_stop_flag) {
                break;
            }

            data_log_parse_chunk(buf, bytes_read);
        }
        fclose(file);

        data_log_parse_finish();
    }
}
#else 
void data_log_parse(const char* fn)
{
    // TODO_COMPRESSION
    // implement the fatfs version of this with f_open, f_read, etc.
    // fatfs docs are here: http://elm-chan.org/fsw/ff/00index_e.html
    // TODO_COMPRESSION
    // open a file for output. David had mentioned prepending a 'c' to the file
    // name to indicate it had been compressed
}
#endif

void data_log_parse_stop(void)
{
    // Set the flag to stop. The main processing loop checks this on each
    // chunk of data.
    g_parse_stop_flag = true;
}

