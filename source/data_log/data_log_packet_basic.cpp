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
#include "micro_clock.h"

/*
 * Basic Packets NO Sample Numbers
 */
// log eeg start info
void data_log_eeg_info(){
#define EEG_INFO_BUFFER_SIZE (PACKET_TYPE_SIZE + sizeof(uint32_t))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(EEG_INFO_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, EEG_INFO_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_EEG_INFO);
  // copy data
  float total_gain = eeg_reader_get_total_gain();
  uint32_t inverse_gain_scalar = total_gain/(double)EEG_ADC2VOLTS_NUMERATOR;
  dlbuf.add(&inverse_gain_scalar, sizeof(inverse_gain_scalar));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef EEG_INFO_BUFFER_SIZE
}


// log commands
void data_log_command(char *line){
#define COMMAND_BUFFER_SIZE (PACKET_TYPE_SIZE + MICRO_TIME_SIZE + 255)
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(COMMAND_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, COMMAND_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_CMD);
  // copy time
  uint64_t time_us = micros();
  dlbuf.add(&time_us, MICRO_TIME_SIZE);
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



/********************************* LOG TIME ********************************************/

#if 0
static void data_log_sample_time(data_log_packet_t packet_type, unsigned long sample_number, float sample_number_hz){
  // TODO: Log the sync time

#define SAMPLE_TIME_BUFFER_SIZE (PACKET_TYPE_SIZE + MICRO_TIME_SIZE + sizeof(packet_type) + SAMPLE_NUMBER_SIZE + sizeof(sample_number_hz) + 255)
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(SAMPLE_TIME_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, SAMPLE_TIME_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_SAMPLE_TIME);
  // copy packet type
  dlbuf.add(&packet_type, sizeof(packet_type));
  // copy time
  uint64_t time_us = micros();
  dlbuf.add(&time_us, MICRO_TIME_SIZE);
  // copy data
  dlbuf.add(&sample_number, SAMPLE_NUMBER_SIZE);
  dlbuf.add(&sample_number_hz, sizeof(sample_number_hz));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef SAMPLE_TIME_BUFFER_SIZE
}

static void data_log_lost_sample_numbers(data_log_packet_t packet_type, unsigned long sample_number_start, unsigned long sample_number_end){
#define LOST_SAMPLE_BUFFER_SIZE (PACKET_TYPE_SIZE + PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + SAMPLE_NUMBER_SIZE)
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(LOST_SAMPLE_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, LOST_SAMPLE_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_LOST_SAMPLES);
  // copy lost packet type
  dlbuf.add(packet_type);
  // copy sample number range
  dlbuf.add(&sample_number_start, SAMPLE_NUMBER_SIZE);
  dlbuf.add(&sample_number_end, SAMPLE_NUMBER_SIZE);
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef LOST_SAMPLE_BUFFER_SIZE
}

class SampleNumberTracker{
private:
  data_log_packet_t packet_type_;
  unsigned long prev_sample_number_;
  float sample_number_hz_;

public:
  SampleNumberTracker(data_log_packet_t packet_type, float sample_number_hz) :
    packet_type_(packet_type), prev_sample_number_(0), sample_number_hz_(sample_number_hz) {}

  void log_time(unsigned long sample_number){
    data_log_sample_time(packet_type_, sample_number, sample_number_hz_);
    prev_sample_number_ = sample_number-1;
  }

  void track_sample_number(unsigned long sample_number){
    if(prev_sample_number_+1 != sample_number){
      data_log_lost_sample_numbers(DLPT_EEG_DATA, prev_sample_number_+1, sample_number);
    }
    prev_sample_number_ = sample_number;
  }

};

// TODO get sample rate from each sensor source
static SampleNumberTracker eeg_snum_tracker(DLPT_EEG_DATA, 250);
static SampleNumberTracker accel_snum_tracker(DLPT_ACCEL_XYZ,25);
static SampleNumberTracker als_snum_tracker(DLPT_ALS,1);
static SampleNumberTracker mic_snum_tracker(DLPT_MIC,1);
static SampleNumberTracker skin_temp_snum_tracker(DLPT_SKIN_TEMP,1);

void data_log_eeg_time(unsigned long sample_number){
  eeg_snum_tracker.log_time(sample_number);
}

void data_log_accel_time(unsigned long sample_number){
  accel_snum_tracker.log_time(sample_number);
}

void data_log_als_time(unsigned long sample_number){
  als_snum_tracker.log_time(sample_number);
}

void data_log_mic_time(unsigned long sample_number){
  mic_snum_tracker.log_time(sample_number);
}

void data_log_skin_temp_time(unsigned long sample_number){
  skin_temp_snum_tracker.log_time(sample_number);
}

void data_log_eeg_track_sample_number(unsigned long sample_number){
  eeg_snum_tracker.track_sample_number(sample_number);
}

void data_log_accel_track_sample_number(unsigned long sample_number){
  accel_snum_tracker.track_sample_number(sample_number);
}

void data_log_als_track_sample_number(unsigned long sample_number){
  als_snum_tracker.track_sample_number(sample_number);
}

void data_log_mic_track_sample_number(unsigned long sample_number){
  mic_snum_tracker.track_sample_number(sample_number);
}

void data_log_skin_temp_track_sample_number(unsigned long sample_number){
  skin_temp_snum_tracker.track_sample_number(sample_number);
}
#endif

/********************************* LOG DATA ********************************************/

// log acceleration
void data_log_accel(unsigned long accel_sample_number, int16_t x, int16_t y, int16_t z){
//  accel_sample_number = (uint32_t) micros() & 0x00000000FFFFFFFFL;

#if (defined(ENABLE_DATA_LOG_STREAM_ACCEL) && (ENABLE_DATA_LOG_STREAM_ACCEL > 0U))
  LOGV("data_log_accel","%lu %d %d %d", (unsigned long) accel_sample_number, (int)x, (int)y, (int)z);
#endif

#define ACCEL_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(x) + sizeof(y) + sizeof(z))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ACCEL_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ACCEL_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_ACCEL_XYZ);
  // copy sample number
  dlbuf.add(&accel_sample_number, SAMPLE_NUMBER_SIZE);
  // copy data
  dlbuf.add(&x, sizeof(x));
  dlbuf.add(&y, sizeof(y));
  dlbuf.add(&z, sizeof(z));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef ACCEL_BUFFER_SIZE
}

// log accelerometer temperature
void data_log_accel_temp(unsigned long accel_sample_number, int16_t temp){
//  accel_sample_number = (uint32_t) micros() & 0x00000000FFFFFFFFL;

#if (defined(ENABLE_DATA_LOG_STREAM_ACCEL_TEMP) && (ENABLE_DATA_LOG_STREAM_ACCEL_TEMP > 0U))
  LOGV("data_log_accel_temp","%lu %d", (unsigned long) accel_sample_number, (int)temp);
#endif

  // Load scratch buffer directly as an optimization
#define ACCEL_TEMP_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(temp))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ACCEL_TEMP_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ACCEL_TEMP_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_ACCEL_TEMP);
  // copy sample number
  dlbuf.add(&accel_sample_number, SAMPLE_NUMBER_SIZE);
  // copy data
  dlbuf.add(&temp, sizeof(temp));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef ACCEL_TEMP_BUFFER_SIZE
}

void data_log_als(unsigned long als_sample_number, float lux) {
  als_sample_number = (uint32_t) micros() & 0x00000000FFFFFFFFL;

#if (defined(ENABLE_DATA_LOG_STREAM_ALS) && (ENABLE_DATA_LOG_STREAM_ALS > 0U))
  LOGV("data_log_als","%lu %f", (unsigned long) als_sample_number, lux);
#endif

#define ALS_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(lux))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ALS_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ALS_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_ALS);
  // copy sample number
  dlbuf.add(&als_sample_number, SAMPLE_NUMBER_SIZE);
  // copy data
  dlbuf.add(&lux, sizeof(lux));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef ALS_BUFFER_SIZE
}

void data_log_mic(unsigned long mic_sample_number, int32_t mic){
  mic_sample_number = (uint32_t) micros() & 0x00000000FFFFFFFFL;

#if (defined(ENABLE_DATA_LOG_STREAM_MIC) && (ENABLE_DATA_LOG_STREAM_MIC > 0U))
  LOGV("data_log_skin_mic","%lu %d", (unsigned long) mic_sample_number, (int) mic);
#endif

#define MEMS_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(mic))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(MEMS_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, MEMS_BUFFER_SIZE);
  // copy packet type
  // TODO: Since mems can sleep between samples, also log a timestamp?
  dlbuf.add(DLPT_MIC);
  // copy sample number
  dlbuf.add(&mic_sample_number, SAMPLE_NUMBER_SIZE);
  // copy data
  dlbuf.add(&mic, sizeof(mic));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef MEMS_BUFFER_SIZE
}

void data_log_skin_temp(unsigned long temp_sample_number, uint8_t* temp_bytes){
  temp_sample_number = (uint32_t) micros() & 0x00000000FFFFFFFFL;

  // Convert 24bit 2's complement to 32 bit 2's complement.
  int32_t temp;
    temp = (temp_bytes[0] << 24
      | temp_bytes[1] << 16
      | temp_bytes[2] << 8) >> 8;

#if (defined(ENABLE_DATA_LOG_STREAM_TEMP) && (ENABLE_DATA_LOG_STREAM_TEMP > 0U))
  LOGV("data_log_skin_temp","%lu %d", (unsigned long) temp_sample_number, (int) temp);
#endif

#define TEMP_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(temp))
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(TEMP_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, TEMP_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_SKIN_TEMP);
  // copy sample number
  dlbuf.add(&temp_sample_number, SAMPLE_NUMBER_SIZE);
  // copy data
//  dlbuf.add(temp_bytes,3);
  dlbuf.add(&temp, sizeof(temp));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef TEMP_BUFFER_SIZE
}

/*
 * Basic Packets with Sample Numbers
 */

// log pulse start and stop
void data_log_pulse(uint32_t eeg_sample_number, bool pulse){
#if (defined(ENABLE_DATA_LOG_STREAM_PULSE) && (ENABLE_DATA_LOG_STREAM_PULSE > 0U))
  LOGV("data_log_pulse","%lu %d", (unsigned long) eeg_sample_number, (int) pulse);
#endif
#define PULSE_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(pulse) )
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(PULSE_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, PULSE_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_PULSE_START_STOP);
  // copy sample number
  dlbuf.add(&eeg_sample_number, SAMPLE_NUMBER_SIZE);
  // copy pulse
  dlbuf.add(&pulse, sizeof(pulse));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef PULSE_BUFFER_SIZE
}

// log echt channel
void data_log_echt_channel(unsigned long eeg_sample_number, uint8_t echt_channel_number){
#if (defined(ENABLE_DATA_LOG_STREAM_ECHT_CHANNEL) && (ENABLE_DATA_LOG_STREAM_ECHT_CHANNEL > 0U))
  LOGV("data_log_echt_channel","%lu %u", (unsigned long) eeg_sample_number, (unsigned int) echt_channel_number);
#endif
#define ECHT_CHANNEL_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(echt_channel_number) )
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(ECHT_CHANNEL_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, ECHT_CHANNEL_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_ECHT_CHANNEL);
  // copy sample number
  dlbuf.add(&eeg_sample_number, SAMPLE_NUMBER_SIZE);
  // copy echt channel number
  dlbuf.add(&echt_channel_number, sizeof(echt_channel_number));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef ECHT_CHANNEL_BUFFER_SIZE
}

// log stimulus switch
void data_log_stimulus_switch(unsigned long eeg_sample_number, bool stim_on){
#if (defined(ENABLE_DATA_LOG_STREAM_STIM_SWITCH) && (ENABLE_DATA_LOG_STREAM_STIM_SWITCH > 0U))
  LOGV("data_log_stim_switch","%lu %d", (unsigned long) eeg_sample_number, (int) stim_on);
#endif
#define STIMULUS_SWITCH_BUFFER_SIZE (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + sizeof(stim_on) )
  uint8_t* scratch = (uint8_t*) dl_malloc_if_file_ready(STIMULUS_SWITCH_BUFFER_SIZE);
  if(scratch==NULL) {return;}
  DLBuffer dlbuf(scratch, STIMULUS_SWITCH_BUFFER_SIZE);
  // copy packet type
  dlbuf.add(DLPT_SWITCH);
  // copy sample number
  dlbuf.add(&eeg_sample_number, SAMPLE_NUMBER_SIZE);
  // copy stim on
  dlbuf.add(&stim_on, sizeof(stim_on));
  // send
  send_data(&dlbuf, portMAX_DELAY);
#undef STIMULUS_SWITCH_BUFFER_SIZE
}

