/*
 * erp.h
 *
 *  Created on: Mar 18, 2022
 *      Author: david
 */

#ifndef EEG_READER_ERP_H_
#define EEG_READER_ERP_H_


#ifdef __cplusplus
extern "C" {
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void erp_pretask_init(void);

void erp_task(void *ignored);

void erp_event_start(uint32_t num_trials, uint32_t pulse_dur_ms, uint32_t isi_ms, uint32_t jitter_ms, uint8_t volume);
void erp_event_stop();

void erp_set_eeg_sample_number(unsigned long sample_number);

#ifdef __cplusplus
}
#endif

#endif /* EEG_READER_ERP_H_ */
