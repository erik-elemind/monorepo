/*
 * data_log_eeg.cpp
 *
 *  Created on: Apr 23, 2022
 *      Author: DavidWang
 */

#include <data_log_packet.h>
#include "data_log_buffer_packed.h"
#include "data_log_internal.h"
#include "data_log.h"
#include "loglevels.h"
#include "data_log_buffer.h"
#include "eeg_reader.h"
#include "eeg_constants.h"

/*
 * Basic Packets NO Sample Numbers
 */
// log eeg start info
void data_log_eeg_start(){
#define EEG_START_BUFFER_SIZE (PACKET_TYPE_SIZE + sizeof(uint32_t))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(EEG_START_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, EEG_START_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_EEG_START);
  // load data
  float total_gain = eeg_reader_get_total_gain();
  uint32_t inverse_gain_scalar = total_gain/(double)EEG_ADC2VOLTS_NUMERATOR;
  dlbuf.add(&inverse_gain_scalar, sizeof(inverse_gain_scalar));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef EEG_START_BUFFER_SIZE
}

// log acceleration
void data_log_accel(int16_t x, int16_t y, int16_t z){
#if (defined(ENABLE_DATA_LOG_STREAM_ACCEL) && (ENABLE_DATA_LOG_STREAM_ACCEL > 0U))
  LOGV("data_log_accel","%d %d %d", (int)x, (int)y, (int)z);
#endif

#define ACCEL_BUFFER_SIZE (PACKET_TYPE_SIZE + sizeof(x) + sizeof(y) + sizeof(z))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ACCEL_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ACCEL_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_ACCEL_XYZ);
  // load data
  dlbuf.add(&x, sizeof(x));
  dlbuf.add(&y, sizeof(y));
  dlbuf.add(&z, sizeof(z));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef ACCEL_BUFFER_SIZE
}

// log accelerometer temperature
void data_log_accel_temp(int16_t temp){
#if (defined(ENABLE_DATA_LOG_STREAM_ACCEL_TEMP) && (ENABLE_DATA_LOG_STREAM_ACCEL_TEMP > 0U))
  LOGV("data_log_accel_temp","%d", (int)temp);
#endif

  // Load scratch buffer directly as an optimization
#define ACCEL_TEMP_BUFFER_SIZE (PACKET_TYPE_SIZE + sizeof(temp))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ACCEL_TEMP_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ACCEL_TEMP_BUFFER_SIZE);
  dlbuf.add(DLPT_ACCEL_TEMP);
  dlbuf.add(&temp, sizeof(temp));
  send_data(&dlbuf, portMAX_DELAY);
#undef ACCEL_TEMP_BUFFER_SIZE
}

// log commands
void data_log_command(char *line){
  //TODO: complete this method
#define COMMAND_BUFFER_SIZE (PACKET_TYPE_SIZE + 255)
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(COMMAND_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, COMMAND_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_CMD);
  // number of characters
  size_t linelen = strlen(line);
  // restrict the line length very conservatively
  // we're trying not to blow the ~256 character limit for COBS.
  uint8_t linelen_uint8 = linelen>=200 ? 200 : linelen;
  dlbuf.add(&linelen_uint8, sizeof(linelen_uint8));
  // string
  dlbuf.add(line, linelen_uint8);
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef COMMAND_BUFFER_SIZE
}

void data_log_als(float lux) {
#if (defined(ENABLE_DATA_LOG_STREAM_ALS) && (ENABLE_DATA_LOG_STREAM_ALS > 0U))
  LOGV("data_log_als","%f", lux);
#endif

#define ALS_BUFFER_SIZE (PACKET_TYPE_SIZE + 255)
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ALS_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ALS_BUFFER_SIZE);
  dlbuf.add(DLPT_ALS);
  dlbuf.add(&lux, sizeof(lux));
  send_data(&dlbuf, portMAX_DELAY);
#undef ALS_BUFFER_SIZE
}

void data_log_mic(int32_t mic){
#if (defined(ENABLE_DATA_LOG_STREAM_MIC) && (ENABLE_DATA_LOG_STREAM_MIC > 0U))
  LOGV("data_log_skin_mic","%d", (int) mic);
#endif

#define MEMS_BUFFER_SIZE (PACKET_TYPE_SIZE + sizeof(mic))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(MEMS_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, MEMS_BUFFER_SIZE);
  // TODO: Since mems can sleep between samples, also log a timestamp?
  dlbuf.add(DLPT_MIC);
  dlbuf.add(&mic, sizeof(mic));
  send_data(&dlbuf, portMAX_DELAY);
#undef MEMS_BUFFER_SIZE
}

void data_log_temp(int32_t temp){
#if (defined(ENABLE_DATA_LOG_STREAM_TEMP) && (ENABLE_DATA_LOG_STREAM_TEMP > 0U))
  LOGV("data_log_skin_temp","%d", (int) temp);
#endif

#define TEMP_BUFFER_SIZE (PACKET_TYPE_SIZE + sizeof(temp))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(TEMP_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, TEMP_BUFFER_SIZE);
  dlbuf.add(DLPT_TEMP);
  dlbuf.add(&temp, sizeof(temp));
  send_data(&dlbuf, portMAX_DELAY);
#undef TEMP_BUFFER_SIZE
}

/*
 * Basic Packets with Sample Numbers
 */

// log pulse start and stop
void data_log_pulse(uint32_t sample_number, bool pulse){
#if (defined(ENABLE_DATA_LOG_STREAM_PULSE) && (ENABLE_DATA_LOG_STREAM_PULSE > 0U))
  LOGV("data_log_pulse","%lu %d", (unsigned long) sample_number, (int) pulse);
#endif
#define PULSE_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(pulse) )
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(PULSE_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, PULSE_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_PULSE_START_STOP);
  // copy sample number
  dlbuf.add(&sample_number, sizeof(sample_number));
  // copy pulse
  dlbuf.add(&pulse, sizeof(pulse));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef PULSE_BUFFER_SIZE
}

// log echt channel
void data_log_echt_channel(unsigned long sample_number, uint8_t echt_channel_number){
#if (defined(ENABLE_DATA_LOG_STREAM_ECHT_CHANNEL) && (ENABLE_DATA_LOG_STREAM_ECHT_CHANNEL > 0U))
  LOGV("data_log_echt_channel","%lu %u", (unsigned long) sample_number, (unsigned int) echt_channel_number);
#endif
#define ECHT_CHANNEL_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(echt_channel_number) )
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ECHT_CHANNEL_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ECHT_CHANNEL_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_ECHT_CHANNEL);
  // copy sample number
  dlbuf.add(&sample_number, sizeof(sample_number));
  // copy echt channel number
  dlbuf.add(&echt_channel_number, sizeof(echt_channel_number));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef ECHT_CHANNEL_BUFFER_SIZE
}

// log stimulus switch
void data_log_stimulus_switch(unsigned long sample_number, bool stim_on){
#if (defined(ENABLE_DATA_LOG_STREAM_STIM_SWITCH) && (ENABLE_DATA_LOG_STREAM_STIM_SWITCH > 0U))
  LOGV("data_log_stim_switch","%lu %d", (unsigned long) sample_number, (int) stim_on);
#endif
#define STIMULUS_SWITCH_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(stim_on) )
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(STIMULUS_SWITCH_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, STIMULUS_SWITCH_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_SWITCH);
  // copy sample number
  dlbuf.add(&sample_number, sizeof(sample_number));
  // copy stim on
  dlbuf.add(&stim_on, sizeof(stim_on));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef STIMULUS_SWITCH_BUFFER_SIZE
}

