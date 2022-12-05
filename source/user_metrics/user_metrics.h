#ifndef USER_METRICS_H
#define USER_METRICS_H

#ifdef __cplusplus
extern "C" {
#endif

void user_metrics_pretask_init(void);
void user_metrics_task(void *ignored);

// Send various event types to this task:
void user_metrics_event_input(void);
void user_metrics_event_stop(void);

#ifdef __cplusplus
}
#endif

#endif  // USER_METRICS_H
