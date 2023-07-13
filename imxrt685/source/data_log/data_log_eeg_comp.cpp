/*
 * data_log_eeg_comp.c
 *
 * Copyright (C) 2022 Elemind Technologies, Inc.
 *
 * Created: Apr, 2022
 * Author:  Bradey Honsinger
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "data_log.h"
#include "data_log_internal.h"
#include "data_log_eeg_comp.h"
#include "data_log_parse.h"
#include "eeg_datatypes.h"
#include "data_log_packet.h"

#define LOG_LEVEL_MODULE    LOG_WARN
#include "loglevels.h"

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

alignas(4) uint8_t g_scratch_buffer[EEG_COMP_BUFFER_SIZE_MAX] = {0};

static const char *TAG = "data_log_eeg_comp";  // Logging prefix for this module

static eeg_comp_t g_eeg_comp_offline;

static uint8_t* get_buffer(size_t scratch_size){
  configASSERT( scratch_size <= sizeof(g_scratch_buffer))
  return g_scratch_buffer;
}

static void write_buffer(uint8_t* buffer, size_t size){
  data_log_write(buffer, size);
}

static void init_eeg_comp() {
  eeg_comp_init(&g_eeg_comp_offline);
  g_eeg_comp_offline.get_buffer = get_buffer;
  g_eeg_comp_offline.write_buffer = write_buffer;
  eeg_comp_reset(&g_eeg_comp_offline);
}

static void compress_and_write_eeg(uint32_t sample_number, int32_t eeg_channels[]) {
  // write the eeg data
  eeg_comp_add_data_and_write(&g_eeg_comp_offline, sample_number, eeg_channels);
}

// Callback when a data log packet is unpacked from the log file to compress
static void packet_cb(const uint8_t* buf, size_t bufsz)
{
    LOGD(TAG, "cobs cb. size=%zu, byte[0]=0x%02x (%u) byte[1]=0x%02x (%d)\n",
        bufsz, buf[0], buf[0], buf[1], buf[1]);

    // First byte is the packet marker (aka type field):
    data_log_packet_t type = (data_log_packet_t)buf[0];
    LOGI(TAG, "cobs decode cb. type=%d (%s), len=%u\n", type, data_log_packet_str(type), bufsz);

    if (type == DLPT_EEG_DATA_PACKED) {
      // Decode and compress EEG data.
      uint32_t idx = 1; // start at the sample num offset
      uint32_t sample_num = *(uint32_t*)&buf[idx];
      idx += sizeof(sample_num);
      int32_t eeg_sample[MAX_NUM_EEG_CHANNELS];
      LOGI(TAG, "  sample_num=%lu\n", sample_num);

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
              if (eeg_sample[j] & 0x00800000) {
                  eeg_sample[j] |= 0xFF000000;
              }
          }

          LOGD(TAG, "  samples=[%ld,%ld,%ld]\n",
              eeg_sample[0], eeg_sample[1], eeg_sample[2]
          );

          compress_and_write_eeg(sample_num + i, eeg_sample);
        }
        // Just sanity checking the index to make sure we didn't exceed the len
        LOGD(TAG, "  idx=%ld\n", idx);
    }
    else {
      // This is a non-EEG sample which does not need further processing.
      // This block should be saved back uncompressed and unmodified.
      data_log_write(buf, bufsz);
    }
}

bool compress_log_eeg(const char* log_filename_to_compress) {
  init_eeg_comp();

  // open file to write (adds prefix to filename)
  open_eeg_comp_data_log(log_filename_to_compress);

  // TODO: Save EEG values left in g_eeg_comp.eegX_buf before returning?
  return data_log_parse(log_filename_to_compress, packet_cb);
}


#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

