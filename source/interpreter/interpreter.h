#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_



#include <stdbool.h>
#include <stdint.h>
//#include <ctime>

#define MAX_PATH_LENGTH 128
#define SCRIPT_DIR_PATH "/scripts"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum therapy_type_t{
  THERAPY_TYPE_NONE = 0,
  THERAPY_TYPE_BASELINE,
  THERAPY_TYPE_THERAPY,
}therapy_type_t;


// Init called before vTaskStartScheduler() launches our Task in main():
void interpreter_pretask_init(void);
void interpreter_task(void *ignored);

void interpreter_event_start_script(const char* filepath);
void interpreter_event_stop_script(bool start_electrode_quality);

void interpreter_event_delay(unsigned long time_ms);
void interpreter_event_start_timer1(unsigned long time_ms);
void interpreter_event_wait_timer1();

// Send various event types to this task:
void interpreter_event_start_therapy(therapy_type_t therapy_type);
void interpreter_event_natural_stop_therapy(void);
void interpreter_event_forced_stop_therapy(void);

void interpreter_event_blink_start_test();
void interpreter_event_blink_stop_test();
void interpreter_event_blink_detected();

#ifdef __cplusplus
}
#endif

#endif  // _INTERPRETER_H_
