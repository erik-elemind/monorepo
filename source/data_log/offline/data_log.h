#pragma once

// minimal info for offline testing.
// copied fields from source_code/data_log/data_log.h

typedef enum data_log_packet_t{
  DLPT_EEG_START = 9,
  DLPT_EEG_DATA = 10,
  DLPT_INST_AMP_PHS = 11,
  DLPT_PULSE_START_STOP = 12,
  DLPT_EEG_DATA_PACKED = 13,
  DLPT_INST_AMP_PHS_PACKED = 14,
  DLPT_ECHT_CHANNEL=15,
  DLPT_SWITCH=16,
  DLPT_STIM_AMP=17,
  DLPT_STIM_AMP_PACKED=18,
  DLPT_CMD=19,
  DLPT_ACCEL_XYZ=20,
  DLPT_ACCEL_TEMP=21,
  DLPT_ALS=22,
  DLPT_MIC=23,
  DLPT_TEMP=24,
  DLPT_EEG_COMP_HEADER=25,
  DLPT_EEG_COMP_FRAME=26,
} data_log_packet_t;

// This is actually from data_log.cpp
#define EEG_PACK_NUM_SAMPLES_TO_SEND (10U)
