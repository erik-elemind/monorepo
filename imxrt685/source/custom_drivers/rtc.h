#ifndef RTC_H_
#define RTC_H_

#include <stdint.h>
#include "ble.h"  // for alarm_params_t

#ifdef __cplusplus
extern "C" {
#endif

// Get the current time, in seconds since Unix epoch.
uint32_t rtc_get(void);

// Set the RTC time.
int rtc_set(uint32_t time);

// Arm the RTC alarm based on the current settings
// This function is not safe to call from ISR context.
int rtc_alarm_init(void);

// Set/update the alarm parameters
int rtc_alarm_set(const alarm_params_t* p_alarm);

// Get/read the alarm parameters
int rtc_alarm_get(alarm_params_t* p_alarm);

// Callback upon rtc alarm triggering.
// This is defined weakly internally, but should be overriden by a callback
// elsewhere in the system.
// This is invoked from ISR context and all processing should be deferred to 
// a proper task.
// IMPORTANT: this routine must trigger a call to rtc_alarm_init() to re-arm 
// the alarm for the next day.
void rtc_alarm_isr_cb(void);

#ifdef __cplusplus
}
#endif

#endif /* RTC_H_ */
