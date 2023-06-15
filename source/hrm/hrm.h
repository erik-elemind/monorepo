#ifndef HRM_H
#define HRM_H

#include "max86140.h"

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

// Init called before vTaskStartScheduler() launches our Task in main():
void hrm_pretask_init(void);
void hrm_task(void *ignored);

// Send various event types to this task:
void hrm_event_turn_off(void);
void hrm_event_turn_on(void);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif  // HRM_H
