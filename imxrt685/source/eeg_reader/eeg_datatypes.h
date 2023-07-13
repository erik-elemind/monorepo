/*
 * eeg_data_record.h
 *
 *  Created on: May 30, 2021
 *      Author: DavidWang
 */

#ifndef EEG_READER_EEG_DATATYPES_H_
#define EEG_READER_EEG_DATATYPES_H_

#include <stddef.h>
#include <stdint.h>
#include "ads129x.h" // needed for #defines "TIMESTAMP_SIZE_IN_BYTES" and "SAMPLE_NUMBER_SIZE_IN_BYTES"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t eeg_channel_t;
typedef uint8_t eeg_order_t;

#define ADS129X_NUM_CHANNELS (8U)
#define MAX_NUM_EEG_CHANNELS (3U)
#define CHANNEL_NAME_MAX_NUM_CHARS (16+1) // add the +1 for the null character


enum {EEG_CH1=0, EEG_CH2, EEG_CH3, EEG_CH4, EEG_CH5, EEG_CH6, EEG_CH7, EEG_CH8};
enum {EEG_FP1=0, EEG_FPZ, EEG_FP2};


typedef struct {
  char          name[CHANNEL_NAME_MAX_NUM_CHARS];
  eeg_channel_t number;
} channel_id_t;

typedef struct {
  eeg_channel_t num_ordering;
  channel_id_t ordering[MAX_NUM_EEG_CHANNELS];
} eeg_channel_config_t;

typedef struct {
  unsigned long timestamp_ms;
  unsigned long eeg_sample_number;
  unsigned long status;
  int32_t eeg_channels[ADS129X_NUM_CHANNELS];
//  union {
//    int32_t channels[ADS129X_NUM_CHANNELS];
//    struct{
//      int32_t ch1;
//      int32_t ch2;
//      int32_t ch3;
//      int32_t ch4;
//      int32_t ch5;
//      int32_t ch6;
//      int32_t ch7;
//      int32_t ch8;
//    };
//  } eeg;
  // TODO: record the concept of overflow and underflow
} ads129x_sample;

typedef struct {
//  unsigned long timestamp_ms;
  unsigned long eeg_sample_number;
//  unsigned long status;
  int32_t eeg_channels[MAX_NUM_EEG_CHANNELS];
  // TODO: record the concept of overflow and underflow
} ads129x_frontal_sample;


void ads_decode_sample(uint8_t* data, size_t data_len, ads129x_sample *sample);

void ads_decode_frontal_sample(uint8_t* data, size_t data_len, ads129x_frontal_sample *sample);

void eeg_channel_config_reset(eeg_channel_config_t *config);

void eeg_channel_config_add(eeg_channel_config_t *config, char* name, eeg_channel_t channel_number );


#if 0
typedef struct {
  unsigned long timestamp_ms;
  unsigned long eeg_sample_number;
  float inst_amp;
  float inst_phs;
  eeg_channel_t channel;
} echt_record_t;

typedef struct {
  echt_record_t inst[MAX_NUM_EEG_CHANNELS];
} echt_records_t;


void echt_encode(eeg_sample_number, float inst_amp, float inst_phs){

}
#endif

#ifdef __cplusplus
}
#endif


#endif /* EEG_READER_EEG_DATATYPES_H_ */
