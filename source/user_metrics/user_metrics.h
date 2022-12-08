#ifndef USER_METRICS_H
#define USER_METRICS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	HYPNOGRAM_DATA,
	HRM_DATA,
	ACTIVITY_DATA
} user_metrics_data_t;

void user_metrics_pretask_init(void);
void user_metrics_task(void *ignored);

// Send various event types to this task:
void user_metrics_event_open(void);
void user_metrics_event_input(uint8_t data, user_metrics_data_t datatype);
void user_metrics_event_stop(void);

#ifdef __cplusplus
}
#endif

#endif  // USER_METRICS_H
