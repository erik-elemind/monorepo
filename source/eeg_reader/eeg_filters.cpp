/*
 * eeg_filters.c
 *
 *  Created on: May 28, 2021
 *      Author: DavidWang
 */

#include "eeg_filters.h"
#include "config.h"


void eeg_filters_init(eeg_filters_context_t *context){
  eeg_filters_enable_line_filter(context, true);
  eeg_filters_enable_az_filter(context, true);
  eeg_filters_config_line_filter(context, 14,25);
  eeg_filters_config_az_filter(context, 14, 0.5);
}

void eeg_filters_enable_line_filter(eeg_filters_context_t *context, bool enable){
  context->enable_line_filters = enable;
}

void eeg_filters_enable_az_filter(eeg_filters_context_t *context, bool enable){
  context->enable_az_filters = enable;
}

void eeg_filters_config_line_filter(eeg_filters_context_t *context, int order, double cutOffFreq,  double sampFreq, bool resetCache){
    int ADCdrop = 0;
    float sampFreqWithDrop = sampFreq / (ADCdrop + 1);
//    context->filter_ch1.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch2.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch3.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch4.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch5.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch6.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch7.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch8.designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
    for(size_t i=0; i<MAX_NUM_EEG_FILTERS; i++){
      context->filters[i].designLineFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
    }
}

void eeg_filters_config_az_filter(eeg_filters_context_t *context, int order, double cutOffFreq, double sampFreq, bool resetCache){
    int ADCdrop = 0;
    float sampFreqWithDrop = sampFreq / (ADCdrop + 1);
//    context->filter_ch1.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch2.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch3.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch4.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch5.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch6.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch7.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
//    context->filter_ch8.designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
    for(size_t i=0; i<MAX_NUM_EEG_FILTERS; i++){
      context->filters[i].designAZFilters(order, cutOffFreq, sampFreqWithDrop, resetCache);
    }
}

void eeg_filters_filter(eeg_filters_context_t *context, ads129x_frontal_sample *sample){
  bool enable_line = context->enable_line_filters;
  bool enable_az = context->enable_az_filters;
//  sample->ch1 = context->filter_ch1.filter(sample->ch1, enable_line, enable_az);
//  sample->ch2 = context->filter_ch2.filter(sample->ch2, enable_line, enable_az);
//  sample->ch3 = context->filter_ch3.filter(sample->ch3, enable_line, enable_az);
//  sample->ch4 = context->filter_ch4.filter(sample->ch4, enable_line, enable_az);
//  sample->ch5 = context->filter_ch5.filter(sample->ch5, enable_line, enable_az);
//  sample->ch6 = context->filter_ch6.filter(sample->ch6, enable_line, enable_az);
//  sample->ch7 = context->filter_ch7.filter(sample->ch7, enable_line, enable_az);
//  sample->ch8 = context->filter_ch7.filter(sample->ch7, enable_line, enable_az);
  for(size_t i=0; i<MAX_NUM_EEG_FILTERS; i++){
    sample->eeg_channels[i] = context->filters[i].filter(sample->eeg_channels[i], enable_line, enable_az);
  }
}



