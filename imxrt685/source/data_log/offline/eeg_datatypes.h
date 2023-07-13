#pragma once

// minimal info for offline testing.
// copied fields from source_code/eeg_reader/eeg_datatypes.h

#include <stdint.h>

#define MAX_NUM_EEG_CHANNELS (3U)

typedef struct {
//  unsigned long timestamp_ms;
  unsigned long sample_number;
//  unsigned long status;
  int32_t eeg_channels[MAX_NUM_EEG_CHANNELS];
  // TODO: record the concept of overflow and underflow
} ads129x_frontal_sample;