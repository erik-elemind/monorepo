#ifndef ML_H
#define ML_H

#ifdef __cplusplus
extern "C" {
#endif


void ml_pretask_init(void);
void ml_task(void *ignored);

// Send various event types to this task:
void ml_event_input(void);

#ifdef __cplusplus
}
#endif

#endif  // ML_H
