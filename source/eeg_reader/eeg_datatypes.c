/*
 * eeg_data_record.c
 *
 *  Created on: May 30, 2021
 *      Author: DavidWang
 */

#include <eeg_datatypes.h>
#include <string.h>


void ads_decode_sample(uint8_t* data, size_t data_len, ads129x_sample *sample){
  memset(sample,0,sizeof(*sample));
  int offset = 0;
  memcpy(&(sample->timestamp_ms),data+offset,TIMESTAMP_SIZE_IN_BYTES);
  offset += TIMESTAMP_SIZE_IN_BYTES;
  memcpy(&(sample->eeg_sample_number),data+offset,SAMPLE_NUMBER_SIZE_IN_BYTES);
  offset += SAMPLE_NUMBER_SIZE_IN_BYTES;
  memcpy(&(sample->status),data+offset,3);
  // save off eeg channel data
  int eeg_channel_index = 0;
  offset += 3;
  for (register int i = offset; i < data_len; i+=3) {
//    LOGV(TAG,"idx: %d, data_len: %d", i, data_len);

    // convert from 3 bytes in 24bit 2's complement to 32bit 2's complement.
    // Note that:
    // * The original over-range code in 24bit 2's complement was:  7FFFFFh
    // * The same over-range code in 32bit 2's complement is:       007FFFFFh
    // * The original under-range code in 24bit 2's complement was: 800000h
    // * The same under-range code in 32bit 2's complement is:      FF800000h
    // Note: The under and over range values are VALID values,
    // but they may indicate clipping of the EEG.
    sample->eeg_channels[eeg_channel_index++] = (
          (data[i]   << 24)
      |   (data[i+1] << 16)
      |   (data[i+2] << 8)
      ) >> 8;
  }
}

void ads_decode_frontal_sample(uint8_t* data, size_t data_len, ads129x_frontal_sample *sample){
  memset(sample,0,sizeof(*sample));
  int offset = 0;
  memcpy(&(sample->eeg_sample_number),data+offset,SAMPLE_NUMBER_SIZE_IN_BYTES);
  offset += SAMPLE_NUMBER_SIZE_IN_BYTES;
  int eeg_channel_index = 0;
  for (register int i = offset; i < data_len; i+=3) {
//    LOGV(TAG,"idx: %d, data_len: %d", i, data_len);

    // convert from 3 bytes in 24bit 2's complement to 32bit 2's complement.
    // Note that:
    // * The original over-range code in 24bit 2's complement was:  7FFFFFh
    // * The same over-range code in 32bit 2's complement is:       007FFFFFh
    // * The original under-range code in 24bit 2's complement was: 800000h
    // * The same under-range code in 32bit 2's complement is:      FF800000h
    // Note: The under and over range values are VALID values,
    // but they may indicate clipping of the EEG.
    sample->eeg_channels[eeg_channel_index++] = (
          (data[i]   << 24)
      |   (data[i+1] << 16)
      |   (data[i+2] << 8)
      ) >> 8;
  }
}


void eeg_channel_config_reset(eeg_channel_config_t *config){
  memset(config, 0, sizeof(*config));
}


static size_t min(size_t a, size_t b){
  return a < b ? a : b;
}


void eeg_channel_config_add(eeg_channel_config_t *config, char* channel_name, eeg_channel_t channel_number ){
  if( (config->num_ordering) < MAX_NUM_EEG_CHANNELS ){
    eeg_channel_t index = (config->num_ordering);
    channel_id_t *channel_id = &(config->ordering[index]);
    // save channel number
    channel_id->number = channel_number;
    // save channel name
    size_t num_chars_to_copy = min( strlen(channel_name), CHANNEL_NAME_MAX_NUM_CHARS-1 );
    memcpy(&(channel_id->name[0]), channel_name, num_chars_to_copy);
    channel_id->name[num_chars_to_copy] = '\0';
    // increment the ordering index
    (config->num_ordering)++;
  }
}



