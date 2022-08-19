#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif


// LED states
typedef enum
{
  LED_OFF = 0,
  LED_ON,
  LED_THERAPY,
  LED_CHARGING,
  LED_CHARGED,
  LED_CHARGE_FAULT,
  LED_RED,
  LED_POWER_GOOD,
  LED_POWER_LOW,
} led_pattern_t;


void set_led_state(led_pattern_t led_pattern);



// Init called before vTaskStartScheduler() launches our Task in main():
void led_pretask_init(void);

void led_task(void *ignored);

// Send various event types to this task:
void led_event_type1(void);
void led_event_type2(void);
void led_event_type3(void);

#ifdef __cplusplus
}
#endif

#endif  // LED_H
