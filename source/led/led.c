

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "pin_mux.h"

#include "loglevels.h"
#include "config.h"
#include "led.h"

#include "anim.h"
#include "audio.h"

static const char *TAG = "led";	// Logging prefix for this module

/******************************************************************************
 * LED GPIO Settings
 *****************************************************************************/

typedef struct
{
	uint32_t port;
	uint32_t pin;
	uint32_t PIO_pwm_output;
	uint32_t PIO_tristate;
} led_ioconfig_t;

led_ioconfig_t g_led_ioconfg_red;
led_ioconfig_t g_led_ioconfg_grn;
led_ioconfig_t g_led_ioconfg_blu;

static inline void led_io_init(led_ioconfig_t *config, uint32_t port, uint32_t pin){
	config->port = port;
	config->pin = pin;
	// Assume the default pin config works with the SC Timer.
	config->PIO_pwm_output = LED_PIO_PERIPHERAL->PIO[port][pin];
	// Change the pin to input, disable the input buffer.
	config->PIO_tristate = (LED_PIO_PERIPHERAL->PIO[port][pin] & ~IOPCTL_PIO_FUNC3 & ~IOPCTL_PIO_INBUF_DI) | IOPCTL_PIO_FUNC0;
}

static inline void led_io_tristate(led_ioconfig_t *ioconfig){
  LED_PIO_PERIPHERAL->PIO[ioconfig->port][ioconfig->pin] = ioconfig->PIO_tristate;
}

static inline void led_io_pwm(led_ioconfig_t *ioconfig){
  LED_PIO_PERIPHERAL->PIO[ioconfig->port][ioconfig->pin] = ioconfig->PIO_pwm_output;
}

/******************************************************************************
 * LED SCTimer Settings
 *****************************************************************************/

static inline void led_sct_enable(SCT_Type *SCTbase, const sctimer_pwm_signal_param_t *pwmSignalsConfig, const uint32_t periodEvent){
    const sctimer_out_t output = pwmSignalsConfig->output;
    const sctimer_pwm_level_select_t level = pwmSignalsConfig->level;
    const uint32_t pulseEvent  = periodEvent + 1;

    // stop PWM timer
    SCTIMER_StopTimer(SCTbase, kSCTIMER_Counter_U);

    if (level == kSCTIMER_LowTrue) {
      // enable events
      SCTbase->OUT[output].CLR |= (1UL << periodEvent);
      SCTbase->OUT[output].SET |= (1UL << pulseEvent);
    }else{
      // enable events
      SCTbase->OUT[output].SET |= (1UL << periodEvent);
      SCTbase->OUT[output].CLR |= (1UL << pulseEvent);
    }

    // start PWM timer
    SCTIMER_StartTimer(SCTbase, kSCTIMER_Counter_U);
}

static inline void led_sct_disable(SCT_Type *SCTbase, const sctimer_pwm_signal_param_t *pwmSignalsConfig, const uint32_t periodEvent){
    const sctimer_out_t output = pwmSignalsConfig->output;
    const sctimer_pwm_level_select_t level = pwmSignalsConfig->level;
    const uint32_t pulseEvent  = periodEvent + 1;

    // stop PWM timer
    SCTIMER_StopTimer(SCTbase, kSCTIMER_Counter_U);

    if (level == kSCTIMER_LowTrue) {
      // If SCTimer PWM is configured for "low-true level" and
      // "kSCTIMER_EdgeAlignedPwm": Set the initial output level
      // to HIGH which is the inactive state.

      // disable events
      SCTbase->OUT[output].CLR &= ~(1UL << periodEvent);
      SCTbase->OUT[output].SET &= ~(1UL << pulseEvent);

      SCTbase->OUTPUT |= (1UL << (uint32_t) output);
    } else {
      // If SCTimer PWM is configured for "high-true level" and
      // "kSCTIMER_EdgeAlignedPwm": Set the initial output level
      // to LOW which is the inactive state.

      // disable events
      SCTbase->OUT[output].SET &= ~(1UL << periodEvent);
      SCTbase->OUT[output].CLR &= ~(1UL << pulseEvent);

      SCTbase->OUTPUT &= ~(1UL << (uint32_t) output);
    }

    // start PWM timer
    SCTIMER_StartTimer(SCTbase, kSCTIMER_Counter_U);
}

/******************************************************************************
 * RED/GREEN/BLUE LED PWM
 *****************************************************************************/

void led_init(){
	led_io_init(&g_led_ioconfg_red, LED_RED_PORT, LED_RED_PIN);
	led_io_init(&g_led_ioconfg_grn, LED_GRN_PORT, LED_GRN_PIN);
	led_io_init(&g_led_ioconfg_blu, LED_BLU_PORT, LED_BLU_PIN);

	anim_init();
}

/*
 *  Controls the duty cycle of an SCTimer controlled PWM to range from 0-100.
 *  Extends the functionality of the "SCTIMER_UpdatePwmDutycycle()" function
 *  provided in "fsl_sctimer.h/c", which only allows PWM ranges from 1-100.
 *
 *  SCTbase - pointer to the board's SCTimer control registers.
 *  pwmSignalsConfig - structure describing the PWM configuration:
 *      output  - which of the SCTimer output pins to control.
 *      level   - whether the pin is pulled HIGH or LOW during the PWM.
 *                kSCTIMER_HighTrue - when the PWM is HIGH, the output is HIGH.
 *                kSCTIMER_LowTrue  - when the PWM is  LOW, the output is HIGH.
 *  ioconfig - structure containing pin io configuration
 *  dutyCyclePercent - percent of time the PWM is HIGH.
 *  event -  an integer index into an array used in the fsl_sctimer.h to
 *           keep track of parameters used to generate the PWM.
 *           The index is created by "SCTIMER_SetupPwm()".
 */
/*
 * Notes:
 *
 * -- The function "SCTIMER_UpdatePwmDutycycle()" provided by "fsl_sctimer.c",
 *    cannot take a 0-value dutycycle. To turn the LED all the way off,
 *    we disable PWM and set the output to a fixed output voltage.
 * -- If the LED cannot be turned all the way off using a fixed voltage,
 *    the leakage current can be prevented by tri-stating the pin.
 *
 */
static void
led_UpdatePwmDutycycle(SCT_Type *SCTbase,
	sctimer_pwm_signal_param_t *pwmSignalsConfig,
	uint32_t periodEvent,
	led_ioconfig_t *ioconfig,
    uint8_t dutyCyclePercent,
	bool tristate_when_0duty)
{
  if (dutyCyclePercent == 0) {
    if (tristate_when_0duty){
      // enable tristate
   	  led_io_tristate(ioconfig);
	}
	// disable pwm
    led_sct_disable(SCTbase, pwmSignalsConfig, periodEvent);
  }else{
	if (dutyCyclePercent > 100) {
      dutyCyclePercent = 100;
    }
	// disable tristate
    led_io_pwm(ioconfig);
    // enable pwm
	led_sct_enable(SCTbase, pwmSignalsConfig, periodEvent);
    // update pwm duty cycle
    SCTIMER_UpdatePwmDutycycle(SCT0, pwmSignalsConfig->output, dutyCyclePercent,  periodEvent);
  }
}

void
led_set_rgb(
  uint8_t red_duty_cycle_percent,
  uint8_t grn_duty_cycle_percent,
  uint8_t blu_duty_cycle_percent,
  bool tristate_when_0duty
  )
{
  // Control the LEDs
  led_UpdatePwmDutycycle(LED_SCT_PERIPHERAL, (sctimer_pwm_signal_param_t*) &LED_SCT_pwmSignalsConfig[LED_SCT_RED_INDEX], LED_SCT_pwmEvent[LED_SCT_RED_INDEX],
		  &g_led_ioconfg_red,
		  red_duty_cycle_percent, tristate_when_0duty);
  led_UpdatePwmDutycycle(LED_SCT_PERIPHERAL, (sctimer_pwm_signal_param_t*) &LED_SCT_pwmSignalsConfig[LED_SCT_GRN_INDEX], LED_SCT_pwmEvent[LED_SCT_GRN_INDEX],
		  &g_led_ioconfg_grn,
		  grn_duty_cycle_percent, tristate_when_0duty);
  led_UpdatePwmDutycycle(LED_SCT_PERIPHERAL, (sctimer_pwm_signal_param_t*) &LED_SCT_pwmSignalsConfig[LED_SCT_BLU_INDEX], LED_SCT_pwmEvent[LED_SCT_BLU_INDEX],
		  &g_led_ioconfg_blu,
		  blu_duty_cycle_percent, tristate_when_0duty);

  // If all the LEDs are OFF, halt the SCTimer to save power.
  if( red_duty_cycle_percent==0 &&
      grn_duty_cycle_percent==0 &&
      blu_duty_cycle_percent==0 ) {
    // Halt the SCTimer.
    LED_SCT_PERIPHERAL->CTRL |= (SCT_CTRL_HALT_L_MASK | SCT_CTRL_HALT_H_MASK);
    // The above code was copied from "fsl_sctimer.c",
    // from the function SCTIMER_Deinit().
    // The SCTIMER_Deinit function can optionally control the clocks,
    // using this snippet of code ensures this function never halts
    // the clocks.
  }else{
    // If the SCT0 peripheral is halted.
    if ( (LED_SCT_PERIPHERAL->CTRL & SCT_CTRL_HALT_L_MASK) != 0 &&
         (LED_SCT_PERIPHERAL->CTRL & SCT_CTRL_HALT_H_MASK) != 0 ) {
      // Start the SCTimer.
      SCTIMER_Init(LED_SCT_PERIPHERAL, &LED_SCT_initConfig);
    }
  }
}

// Convert byte value (0-255) to percent value (0-100)
#define BYTE_TO_PERCENT(X) (((X) * 100)/255)

void
led_set_rgb32(uint32_t rgb_duty_cycle_bytes, bool tristate_when_0duty)
{
  led_set_rgb(
    BYTE_TO_PERCENT((rgb_duty_cycle_bytes & 0x00FF0000) >> 16),
    BYTE_TO_PERCENT((rgb_duty_cycle_bytes & 0x0000FF00) >> 8),
    BYTE_TO_PERCENT((rgb_duty_cycle_bytes & 0x000000FF)),
	tristate_when_0duty );
}

// Animation pattern selector.

// Do a smooth transition between colors v1 and v2 over ms
// milliseconds, and then back again over ms milliseconds.  Repeats
// continually.

// v1 & v2 format is the same as Board_SetRGB32: 0x00RRGGBB
static void anim_pulse(uint32_t v1, uint32_t v2, uint32_t ms) {
  static animation_t anim[3][2];
  
  for(int i = 0; i < 3; i++) {
    anim[i][0].from = (v1 >> (8 * i)) & 0xff;
    anim[i][1].to = (v1 >> (8 * i)) & 0xff;
    
    anim[i][1].to = (v2 >> (8 * i)) & 0xff;
    anim[i][0].from = (v2 >> (8 * i)) & 0xff;
    
    anim[i][0].duration_in_ms = ms;
    anim[i][1].duration_in_ms = ms;

    // Shift amounts for Board_SetRGB32.
    anim[i][0].user_data = (void*)(i*8);
    anim[i][1].user_data = (void*)(i*8);

    anim_channel_start(i, anim[i], 2, true);
  }
}

// Callback to set animation color.

void anim_set_value(int value, void *user_data) {
  static uint32_t color_triple = 0;
  color_triple &= ~(0xff << (int)user_data);
  color_triple |= value << (int)user_data;
  led_set_rgb32(color_triple, true);
}

 char* get_pattern_name(led_pattern_t pattern) {
  switch (pattern) {
   case LED_OFF_NO_TRISTATE:  return "LED_OFF_NO_TRISTATE";
   case LED_OFF:  return "LED_OFF";
   case LED_RED:  return "LED_RED";
   case LED_GREEN:  return "LED_GREEN";
   case LED_BLUE:  return "LED_BLUE";
   case LED_ON:  return "LED_ON";
   case LED_THERAPY:  return "LED_THERAPY";
   case LED_CHARGING:  return "LED_CHARGING";
   case LED_CHARGED:  return "LED_CHARGED";
   case LED_CHARGE_FAULT:  return "LED_CHARGE_FAULT";
   case LED_POWER_GOOD: return "LED_POWER_GOOD";
   case LED_POWER_LOW: return "LED_POWER_LOW";
   default: return "UNKNOWN LED PATTERN";
  }
}

// FF3: Note colors are RGY, pre-FF3 are RGB.
static led_pattern_t cur_state;
void
set_led_state(led_pattern_t led_pattern)
{
  LOGD(TAG, "Setting led state to %s\n\r", get_pattern_name(led_pattern));
  
  // Don't reset the animation progress if the state is the same
  if (cur_state == led_pattern) {
    return;
  }
  cur_state = led_pattern;
  
  anim_channel_stop_all();
  switch (led_pattern) {
  	case LED_OFF_NO_TRISTATE:
  	  led_set_rgb32(0x000000, false);
  	  break;
    case LED_OFF:
      led_set_rgb32(0x000000, true);
      break;
    case LED_RED:
      led_set_rgb32(0xFF0000, true);
      break;
    case LED_GREEN:
      led_set_rgb32(0x00FF00, true);
      break;
    case LED_BLUE:
      led_set_rgb32(0x0000FF, true);
      break;
    case LED_ON:
      led_set_rgb32(0x0000aa, true);
      break;
    case LED_THERAPY:
      anim_pulse(0x000000, 0x00aa00, 1000);
      break;
    case LED_CHARGING:
      anim_pulse(0x000000, 0x885500, 1000);
      break;
    case LED_CHARGED:
      led_set_rgb32(0x00ff00, true);
      break;
    case LED_CHARGE_FAULT:
      anim_pulse(0x000000, 0xaaaa00, 200);
      break;
    case LED_POWER_GOOD:
      led_set_rgb32(0x00aa00, true);
      break;
    case LED_POWER_LOW:
      led_set_rgb32(0xaa0000, true);
      break;
    default:
      LOGE(TAG, "set_led_state(): Unknown LED state!\n\r");
      break;
  }
}

