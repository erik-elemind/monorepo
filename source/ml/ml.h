#ifndef ML_H
#define ML_H

#ifdef __cplusplus
extern "C" {
#endif

#include "eeg_datatypes.h"

void ml_pretask_init(void);
void ml_task(void *ignored);

// Send various event types to this task:
void ml_event_input(ads129x_frontal_sample* f_sample);
void ml_event_stop(void);
void ml_enable(void);
void ml_disable(void);

#ifdef __cplusplus
}
#endif

#endif  // ML_H
