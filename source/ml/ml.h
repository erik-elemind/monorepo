#ifndef ML_H
#define ML_H

#include "eeg_datatypes.h"
#include "lis2dtw12.h"
#include "test_model.h" // TODO: Will replace with real model later on
#include "glow_bundle_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILT_TYPE float

void ml_pretask_init(void);
void ml_task(void *ignored);

// Send various event types to this task:
void ml_event_eeg_input(ads129x_frontal_sample* f_sample);
void ml_event_hr_input(uint8_t hr_sample);
void ml_event_acc_input(lis2dtw12_sample_t* acc_sample);

void ml_event_stop(void);
void ml_enable(void);
void ml_disable(void);



// add the other filters with the same template
// ideally we'd just have one big "filter" class but that's for later

#ifdef __cplusplus
}
#endif

#endif  // ML_H
