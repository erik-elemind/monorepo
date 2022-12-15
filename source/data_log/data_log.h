/*
 * data_log.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Oct, 2020
 * Author:  David Wang
 *
 * Description: Implements the logging interface
 */

#ifndef DATA_LOG_H
#define DATA_LOG_H

//#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>  // uint32_t, etc.
#include "ads129x.h"
#include "eeg_datatypes.h"
#include "config.h"
#include "ff.h"


// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void data_log_pretask_init(void);

void data_log_task(void *ignored);

void data_log_open();
void data_log_close();
void user_metrics_log_open(FIL *file);

void data_log_set_time(char *datetime_string, size_t datetime_size);

// log device gain settings
void data_log_eeg_info();
// log eeg data
void data_log_eeg( ads129x_frontal_sample *f_sample );
// log amplitude and phase
void data_log_inst_amp_phs(unsigned long eeg_sample_number, float instAmp, float instPhs);
// log pulse start and stop
void data_log_pulse(unsigned long eeg_sample_number, bool pulse);
// log echt channel
void data_log_echt_channel(unsigned long eeg_sample_number, uint8_t echt_channel_number);
// log stimulus switch
void data_log_stimulus_switch(unsigned long eeg_sample_number, bool stim_on);
// log stimulus amplitude
void data_log_stimulus_amplitude(unsigned long eeg_sample_number, float stim_amp);

// log acceleration
void data_log_accel(unsigned long accel_sample_number, int16_t x, int16_t y, int16_t z);
// log accelerometer temperature
void data_log_accel_temp(unsigned long accel_sample_number, int16_t temp);

// log als
void data_log_als(unsigned long als_sample_number, float lux);

// log mems
void data_log_mic(unsigned long mic_sample_number, int32_t mic);

// log skin temp sensor
void data_log_skin_temp(unsigned long temp_sample_number, uint8_t* temp_bytes);

// log commands
void data_log_command(char* line);

// start compression of existing file
void data_log_compress(char* log_filename_to_compress);

// check compression status
// - sets is_compressing flag if still compressing, else false
// - sets result to last compression result (true on success)
// - sets stats flags to indicate progress
void data_log_compress_status(bool* is_compressing, bool* result, size_t* bytes_in_file, size_t* bytes_processed);

// abort compression (no effect if not currently compressing)
void data_log_compress_abort();


// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif  // DATA_LOG_H
