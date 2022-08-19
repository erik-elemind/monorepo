#include "rtc.h"
#include "fsl_rtc.h"
#include "config.h"
#include "loglevels.h"
#include "settings.h"

#include <stdio.h> // for sprintf()

#define SECONDS_PER_MINUTE  (60)
#define SECONDS_PER_HOUR    (60*60)
#define SECONDS_PER_DAY     (24*SECONDS_PER_HOUR)
// Define the minimum valid date. 
// We use this to determine if the RTC value is valid.
#define DATETIME_MIN        (1577836800)  // 2020-01-01 00:00:00 (see epochconverter.com)

typedef enum {
  DAY_SUN,
  DAY_MON,
  DAY_TUE,
  DAY_WED,
  DAY_THU,
  DAY_FRI,
  DAY_SAT,
  NUM_DAYS
} day_of_week_t;

// Unix time starts on a Thursday
#define UNIX_EPOCH_DAY_OF_WEEK  (DAY_THU)

/// Logging prefix
static const char* TAG = "rtc";

static bool rtc_is_valid(const uint32_t seconds) {
  return seconds >= DATETIME_MIN;
}

static day_of_week_t get_day_of_week(const uint32_t seconds) {
  uint32_t days_since_epoch = seconds / SECONDS_PER_DAY;
  
  // Unix time starts mid-week, so add those days back
  days_since_epoch += UNIX_EPOCH_DAY_OF_WEEK;

  // All weeks have 7 days, so just mod to get the current day
  return days_since_epoch % NUM_DAYS;
}

uint32_t rtc_get(void) {
  return RTC_GetSecondsTimerCount(RTC);
}

int rtc_set(uint32_t time) {
  // Disable the RTC when making changes
  #if 0
  // This hangs:
  RTC_Deinit(RTC);
  RTC_SetSecondsTimerCount(RTC, time);
  RTC_Init(RTC);
  #else

  // Note this is a 32b register and will roll over on 2038-01-19 at 03:14:07 UTC.
  RTC->CTRL &= ~RTC_CTRL_RTC_EN_MASK; // Timer must be stopped before setting COUNT.
  RTC->COUNT = time;
  RTC->CTRL |= RTC_CTRL_RTC_EN_MASK; // Restart.
  #endif

  // Recalibrate the alarm with the new time, if the alarm is set
  rtc_alarm_init();
  return 0;
}

int rtc_alarm_init(void) {
  uint32_t seconds = RTC_GetSecondsTimerCount(RTC);

  // Confirm the RTC is valid before doing anything
  if (!rtc_is_valid(seconds)) {
    LOGE(TAG, "rtc not valid. time=%lu\r\n", seconds);
    return -1;
  }

  // Get the current alarm config, if any
  alarm_params_t alarm;
  if (rtc_alarm_get(&alarm)) {
    return -1;
  }

  // Effectively disable the match event by setting it back to 0
  RTC_SetSecondsTimerMatch(RTC,0);

  if (!alarm.flags.on) {
    // alarm disabled, nothing to do
    return 0;
  }

  // Remap alarm_params_t to day_of_week_t order
  // This simplifies the logic below
  const uint8_t flags_dow = 
    alarm.flags.sun << DAY_SUN |
    alarm.flags.mon << DAY_MON |
    alarm.flags.tue << DAY_TUE |
    alarm.flags.wed << DAY_WED |
    alarm.flags.thu << DAY_THU |
    alarm.flags.fri << DAY_FRI |
    alarm.flags.sat << DAY_SAT;

  day_of_week_t day = get_day_of_week(seconds);
  uint32_t seconds_since_midnight = seconds % SECONDS_PER_DAY;
  uint32_t midnight = seconds - seconds_since_midnight;

  // Find the next valid alarm
  // Loop 7 days starting from today, plus 1 day for the special case where
  // the alarm is enabled for only the current day of week.
  for (unsigned i=0; i<NUM_DAYS+1; i++, day++, midnight+=SECONDS_PER_DAY) {
    // Handle roll over into next week
    if (day >= NUM_DAYS) {
      day -= NUM_DAYS;
    }

    // Is the alarm enabled for the day in question?
    if (flags_dow & (1<<day)) {
      uint32_t seconds_alarm = midnight + alarm.minutes_after_midnight * SECONDS_PER_MINUTE;
      if (seconds >= seconds_alarm) {
        // We've already gone past the alarm time for today.
        // This can only happen on the first iteration of the loop.);
      }
      else {
        // Found a valid alarm, let's set it!
        RTC_SetSecondsTimerMatch(RTC, seconds_alarm);
        LOGI(TAG, "alarm set to %lu (day=%u, minutes=%u). (currently %lu, day=%u, minutes=%lu)\r\n",
            seconds_alarm, day, alarm.minutes_after_midnight, 
            seconds, get_day_of_week(seconds), (seconds_since_midnight/SECONDS_PER_MINUTE));
        return 0;
      }
    }
  }

  LOGW(TAG, "no valid alarm found. flags=0x%x\r\n", alarm.flags.byte);
  return -1;
}

int rtc_alarm_set(const alarm_params_t* p_alarm) {
  if (settings_set_long("alarm-flags", (uint32_t)p_alarm->flags.byte)) {
    return -1;
  }  
  if (settings_set_long("alarm-minutes", (uint32_t)p_alarm->minutes_after_midnight)) {
    return -1;
  }

  // Reinitialize the alarm to ensure it is set with the new params.
  // If time is not set and this fails, we will initialize it later.
  (void)rtc_alarm_init();

  return 0;
}

int rtc_alarm_get(alarm_params_t* p_alarm) {
  long value;
  if (settings_get_long("alarm-flags", &value)) {
    return -1;
  }
  p_alarm->flags.byte = value;

  if (settings_get_long("alarm-minutes", &value)) {
    return -1;
  }
  p_alarm->minutes_after_midnight = value;
  return 0;
}

__WEAK void rtc_alarm_isr_cb(void) {
  // Caller should implement this function.
  // Implementation should be a simple message to the task to take action.
  // Note that we do not call rtc_alarm_init() to re-arm the alarm, but the
  // caller's implementation should do so (outside of ISR context of course).
}

/* RTC_IRQn interrupt handler */
void RTC_IRQHANDLER(void) {
  uint32_t flags = RTC_GetStatusFlags(RTC);

  // Make sure this is the alarm event
  if (flags & kRTC_AlarmFlag) {
    rtc_alarm_isr_cb();
  }

  // Clear the flags so we don't re-enter
  RTC_ClearStatusFlags(RTC, flags);

  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
     Store immediate overlapping exception return operation might vector to incorrect interrupt. */
  #if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
  #endif
}

