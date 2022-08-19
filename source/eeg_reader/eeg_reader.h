/*
 * eeg_reader.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Aug, 2020
 * Author:  David Wang
 */

#ifndef EEG_READER_H
#define EEG_READER_H

#include <stddef.h>
#include "eeg_datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void eeg_reader_pretask_init(void);

void eeg_reader_task(void *ignored);

// Send various event types to this task:
void eeg_reader_event_get_info(void);
void eeg_reader_event_power_on(void);
void eeg_reader_event_power_off(void);
void eeg_reader_event_start(void);
void eeg_reader_event_stop(void);
void eeg_reader_event_set_ads_gain(uint8_t ads_gain);
void eeg_reader_event_temp_sample(int32_t temp);

float eeg_reader_get_total_gain();

#ifdef __cplusplus
}
#endif


#endif  // EEG_READER_H
