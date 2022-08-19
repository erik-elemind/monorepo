/*
 * eeg_reader.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Aug, 2020
 * Author:  David Wang
 */

#ifndef EEG_PROCESSOR_H
#define EEG_PROCESSOR_H

#include <stddef.h>
#include "eeg_datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USE_3CHANNEL_EEG (1U)

#if (defined(USE_3CHANNEL_EEG) && (USE_3CHANNEL_EEG > 0U))
#define EEG_MSG_LEN (4+3*3) // 4 sample number, 3 channels of 3 byte data = 13 bytes
#else
#define EEG_MSG_LEN (4+4+3+8*3) // 4 time stamp, 4 sample number, 3 status, 8 channels of 3 byte data = 35 bytes
#endif


// Init called before vTaskStartScheduler() launches our Task in main():
void eeg_processor_pretask_init(void);

void eeg_processor_task(void *ignored);

uint8_t* eeg_processor_send_eeg_data_open_from_isr(size_t data_len, BaseType_t *pxHigherPriorityTaskWoken);
void eeg_processor_send_eeg_data_close_from_isr(uint8_t* data, size_t data_len, BaseType_t *pxHigherPriorityTaskWoken);

void eeg_processor_init(void);

void eeg_processor_start_quality_check(void);
void eeg_processor_stop_quality_check(void);

void eeg_processor_start_blink_test(void);
void eeg_processor_stop_blink_test(void);

void eeg_processor_enable_line_filters(bool enable);
void eeg_processor_enable_az_filters(bool enable);

void eeg_processor_config_line_filter(int order, double cutfreq);
void eeg_processor_config_az_filter(int order, double cutfreq);

void eeg_processor_config_echt(int fftSize, int filtOrder, float centerFreq, float lowFreq, float highFreq, float inputScale, float sampFreqWithDrop);
void eeg_processor_config_echt_simple(float center_freq);
void eeg_processor_set_echt_channel(eeg_channel_t channel_number);
void eeg_processor_set_echt_min_max_phase(float min_phase_rad, float max_phase_rad);
void eeg_processor_start_echt();
void eeg_processor_stop_echt();

void eeg_processor_enable_alpha_switch(bool enable);
void eeg_processor_config_alpha_switch(float dur1_sec, float dur2_sec, float timedLockOut_sec, float minRMSPower_uV, float alphaThr_dB);


#ifdef __cplusplus
}
#endif


#endif  // EEG_PROCESSOR_H
