/*
 * eeg_filters.h
 *
 *  Created on: May 28, 2021
 *      Author: DavidWang
 */

#ifndef EEG_READER_EEG_FILTERS_H_
#define EEG_READER_EEG_FILTERS_H_

#include <eeg_datatypes.h>
#include "eeg_filter.h"
#include "ads129x.h"


#ifndef MAX_NUM_EEG_FILTERS
#define MAX_NUM_EEG_FILTERS MAX_NUM_EEG_CHANNELS
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define FILT_TYPE float
#define MAX_FILT_ORDER 14


typedef struct
{
  eeg_filter<FILT_TYPE, MAX_FILT_ORDER> filters[MAX_NUM_EEG_FILTERS];
  bool enable_line_filters = false;
  bool enable_az_filters = false;
} eeg_filters_context_t;

void eeg_filters_init(eeg_filters_context_t *context);
void eeg_filters_enable_line_filter(eeg_filters_context_t *context, bool enable);
void eeg_filters_enable_az_filter(eeg_filters_context_t *context, bool enable);
void eeg_filters_config_line_filter(eeg_filters_context_t *context, int order, double cutOffFreq,  double sampFreq=250, bool resetCache = true);
void eeg_filters_config_az_filter(eeg_filters_context_t *context, int order, double cutOffFreq, double sampFreq=250, bool resetCache = true);
void eeg_filters_filter(eeg_filters_context_t *context, ads129x_frontal_sample *sample);

#ifdef __cplusplus
}
#endif

#endif /* EEG_READER_EEG_FILTERS_H_ */
