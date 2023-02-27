#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED states
typedef enum
{
  LED_OFF_TRISTATE = 0,
  LED_OFF,
  LED_RED,
  LED_GREEN,
  LED_BLUE,
  LED_ON,
  LED_THERAPY,
  LED_CHARGING,
  LED_CHARGED,
  LED_CHARGE_FAULT,
  LED_POWER_GOOD,
  LED_POWER_LOW,
} led_pattern_t;


void led_init(void);

/** Set RGB LED to specified color.

    Sets LED color using three percentage values.

    @param red_duty_cycle_percent Percent brightness for red LED (0-100)
    @param green_duty_cycle_percent Percent brightness for green LED (0-100)
    @param blue_duty_cycle_percent Percent brightness for blue LED (0-100)
 */
void
led_set_rgb(
  uint8_t red_duty_cycle_percent,
  uint8_t grn_duty_cycle_percent,
  uint8_t blu_duty_cycle_percent,
  bool tristate_when_0duty);

/** Set RGB LED to specified color using a single value for convenience.

    @param rgb_duty_cycle_bytes RGB value encoded as 0x00RRGGBB, i.e.
    red is byte 2, green is byte 1, blue is byte 0 (LSB). Note that
    RGB byte values (0-255) are scaled to percentage values (0-100)
    for display.
 */
void
led_set_rgb32(uint32_t rgb_duty_cycle_bytes, bool tristate_when_0duty);

void set_led_state(led_pattern_t led_pattern);


#ifdef __cplusplus
}
#endif

#endif  // LED_H
