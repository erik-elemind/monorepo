#include "eeg_sqw.h"

#include "FreeRTOS.h"
#include "timers.h"
#include "loglevels.h"
#include "board_config.h"

static const char *TAG = "eeg_sqw";	// Logging prefix for this module

#if defined(VARIANT_FF3)
static uint8_t g_duty_cycle = 0;
static uint8_t g_freq = 0;
static TimerHandle_t g_sqw_timer_handle;
static StaticTimer_t g_sqw_timer_struct;
static TickType_t g_onPeriod;
static TickType_t g_offPeriod;
static bool g_high = false;

static void sqw_timeout(TimerHandle_t timer_handle);

int set_eeg_sqw(uint8_t freq, uint8_t duty_cycle) {
  if (freq > 100) {
    LOGE(TAG, "SQW invalid frequency");
    return -1;
  }
  if (duty_cycle > 100) {
    LOGE(TAG, "SQW invalid duty_cycle");
    return -1;
  }
  
  if (g_freq) {
    // If we were previously on, reset everything.
    xTimerStop(g_sqw_timer_handle, 0);
    g_high = false;
    GPIO_PinWrite(GPIO, BOARD_INITPINS_SQW_PORT, BOARD_INITPINS_SQW_PIN, 0);
  }
  
  g_freq = freq;
  g_duty_cycle = duty_cycle;

  if (g_freq) {
    // Calculate new tick intervals.
    TickType_t interval = pdMS_TO_TICKS(1000/freq);
    g_onPeriod = (interval*duty_cycle) / 100;
    g_offPeriod = interval - g_onPeriod;
  
    // If we are now on, turn on the timer.
    sqw_timeout(g_sqw_timer_handle);
  } 

  return 0;
}

static void
sqw_timeout(TimerHandle_t timer_handle){
  if (g_high) {
    xTimerChangePeriod(g_sqw_timer_handle, g_offPeriod, 0);
  } else {
    xTimerChangePeriod(g_sqw_timer_handle, g_onPeriod, 0);
  }
  g_high = !g_high;
  GPIO_PinWrite(GPIO, BOARD_INITPINS_SQW_PORT, BOARD_INITPINS_SQW_PIN, g_high);
  xTimerStart(g_sqw_timer_handle, 0);
}

void eeg_sqw_init(void) {
  GPIO_PinWrite(GPIO, BOARD_INITPINS_SQW_PORT, BOARD_INITPINS_SQW_PIN, 0);
  g_sqw_timer_handle = xTimerCreateStatic("SQW_TIMER", pdMS_TO_TICKS(100), pdFALSE, NULL, sqw_timeout,
						  &(g_sqw_timer_struct));
}

#else // defined(VARIANT_FF3)

int set_eeg_sqw(uint8_t freq, uint8_t duty_cycle) {
  LOGE(TAG, "SQW not implemented");
  return -1;
}

void eeg_sqw_init(void) {}

#endif // defined(VARIANT_FF3)
