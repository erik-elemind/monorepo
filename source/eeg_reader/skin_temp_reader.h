/*
 * skin_temp_reader.h
 *
 *  Created on: Aug 7, 2022
 *      Author: DavidWang
 */

#ifndef EEG_READER_SKIN_TEMP_READER_H_
#define EEG_READER_SKIN_TEMP_READER_H_

#include "config.h"
#include "eeg_datatypes.h"


#define TEMP_SENSOR_CHANNEL EEG_CH4

/* Sample every 250th sample of the EEG */
// TODO: Make this value 1HZ, and calculated based on EEG sample rate
#define TEMP_SENSOR_SAMPLE_RATE 250

typedef struct{
  unsigned int temp_sample_number;
  uint8_t temp[3];
} skin_temp_sample_t;


#endif /* EEG_READER_SKIN_TEMP_READER_H_ */
