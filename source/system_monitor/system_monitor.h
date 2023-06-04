#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include "battery_charger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALS_TIMER_MS 1000

#define MIC_TIMER_MS 1000
// Samples before sleeping, waiting for mems-wake interrupt.  Setting
// to 0 will never sleep, and always sample at MIC_TIMER_MS.
#define MIC_SAMPLES_BEFORE_SLEEP 1

// Init called before vTaskStartScheduler() launches our Task in main():
void system_monitor_pretask_init(void);

void system_monitor_task(void *ignored);

// Battery charger event (called by app)
void system_monitor_event_battery(void);

// Battery charger event (triggered by interrupt)
void system_monitor_event_battery_from_isr(void);

// Power off
void system_monitor_event_power_off(void);

// ALS
void system_monitor_event_als_start_sample(unsigned int sample_period_ms);
void system_monitor_event_als_stop(void);

// MIC
void system_monitor_event_mic_start_sample(unsigned int sample_period_ms);
void system_monitor_event_mic_start_thresh(unsigned int sample_period_ms, unsigned int sample_num);
void system_monitor_event_mic_stop(void);

// Read the microphone value once
int32_t system_monitor_event_mic_read_once(void);

// mic sound event (triggered by interrupt)
void system_monitor_event_mic_from_isr(void);

#ifdef __cplusplus
}
#endif

#endif  // SYSTEM_MONITOR_H
