/*
 * board_config.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Morpheus board configuration.
 */
#include "board_config.h"
#include "fsl_sctimer.h"
#include "fsl_debug_console.h"
#include "syscalls.h"  // For Nanolib shell _read()/_write()

// Declare global HAL handles here.
//
// These globals are initialized below in board_init().
// These globals are exported to modules for use as extern in board_config.h.


/******************************************************************************
 * BLUETOOTH
 *****************************************************************************/


int
BOARD_InitBLE()
{
  //
  // USART_BLE init (uses the FreeRTOS driver)
  //
  NVIC_SetPriority(USART_BLE_IRQn, USART_BLE_NVIC_PRIORITY);
  EnableIRQ(USART_BLE_IRQn);

  //RESET_ClearPeripheralReset(USART_BLE_RST);  // TODO: Is this necessary?
  return kStatus_Success;
}



/******************************************************************************
 * DEBUG UART
 *****************************************************************************/
int
BOARD_InitDebugConsole()
{
#ifndef CONFIG_USE_SEMIHOSTING
  // Initialize the Nanolib C system calls (_read(), _write(), _open(), etc.)
  syscalls_pretask_init();
#endif

  NVIC_SetPriority(USART_DEBUG_IRQn, USART_DEBUG_NVIC_PRIORITY);
  EnableIRQ(USART_DEBUG_IRQn);

  return kStatus_Success;
}

/******************************************************************************
 * SPI Busses
 *****************************************************************************/

void
BOARD_InitEEGSPI()
{
}

void
BOARD_InitFlashSPI()
{
}


/******************************************************************************
 * I2C on FLEXCOMM 4
 *****************************************************************************/

/******************************************************************************
 * I2C on FLEXCOMM 5
 *****************************************************************************/

/******************************************************************************
 * RED/GREEN/BLUE LED PWM
 *****************************************************************************/

/*
 * ToDo: Delete the implementation of "BOARD_SetRGB()"
 * when the new implementation is known to work.
 *
 * The original implementation below will never fully turn the LED off.
 * It allows a duty_cycle_percent in the range of 0-100 to be used,
 * but using a duty_cycle_percent of 0% just means 1% is used.
 */

#if 0
void
BOARD_SetRGB(uint8_t red_duty_cycle_percent, uint8_t grn_duty_cycle_percent, uint8_t blu_duty_cycle_percent)
{
  // ToDo: Right now, duty_cycle only ranges from 1-100.
  // There needs to be a way to turn the LEDs off completely.
  // ToDo: Make this method interrupt safe?
  red_duty_cycle_percent = MAX(1,red_duty_cycle_percent);
  SCTIMER_UpdatePwmDutycycle(SCT0, SCTIMER_OUT_RED_LED, red_duty_cycle_percent,  SCT0_pwmEvent[RED_LED_EVENT]);
  grn_duty_cycle_percent = MAX(1,grn_duty_cycle_percent);
  SCTIMER_UpdatePwmDutycycle(SCT0, SCTIMER_OUT_GRN_LED, grn_duty_cycle_percent,  SCT0_pwmEvent[GRN_LED_EVENT]);
  blu_duty_cycle_percent = MAX(1,blu_duty_cycle_percent);
  SCTIMER_UpdatePwmDutycycle(SCT0, SCTIMER_OUT_BLU_LED, blu_duty_cycle_percent,  SCT0_pwmEvent[BLU_LED_EVENT]);
}

#else

/*
 *  Controls the duty cycle of an SCTimer controlled PWM to range from 0-100.
 *  Extends the functionality of the "SCTIMER_UpdatePwmDutycycle()" function
 *  provided in "fsl_sctimer.h/c", which only allows PWM ranges from 1-100.
 *
 *  base   - pointer to the board's SCTimer control registers.
 *  output - which of the SCTimer output pins to control.
 *  level  - whether the pin is pulled HIGH or LOW during the PWM.
 *           kSCTIMER_HighTrue - when the PWM is HIGH, the output is HIGH.
 *           kSCTIMER_LowTrue  - when the PWM is  LOW, the output is HIGH.
 *  dutyCyclePercent - percent of time the PWM is HIGH.
 *  event -  an integer index into an array used in the fsl_sctimer.h to
 *           keep track of parameters used to generate the PWM.
 *           The index is created by "SCTIMER_SetupPwm()".
 */
void
BOARD_UpdatePwmDutycycle(SCT_Type *base,
    sctimer_out_t output,
    sctimer_pwm_level_select_t level,
    uint8_t dutyCyclePercent,
    uint32_t event)
{
  uint32_t whichIO = output;
  uint32_t periodEvent = event;
  uint32_t pulseEvent  = periodEvent + 1;
  if (dutyCyclePercent == 0) {
    // stop PWM timer
    SCTIMER_StopTimer(LED_PERIPHERAL, kSCTIMER_Counter_U);

    // set the output
    if (level == kSCTIMER_LowTrue) {
      // If SCTimer PWM is configured for "low-true level" and
      // "kSCTIMER_EdgeAlignedPwm": Set the initial output level
      // to HIGH which is the inactive state.

      // disable events
      base->OUT[whichIO].CLR &= ~(1UL << periodEvent);
      base->OUT[whichIO].SET &= ~(1UL << pulseEvent);

      base->OUTPUT |= (1UL << (uint32_t) output);
    }
    else {
      // If SCTimer PWM is configured for "high-true level" and
      // "kSCTIMER_EdgeAlignedPwm": Set the initial output level
      // to LOW which is the inactive state.

      // disable events
      base->OUT[whichIO].SET &= ~(1UL << periodEvent);
      base->OUT[whichIO].CLR &= ~(1UL << pulseEvent);

      base->OUTPUT &= ~(1UL << (uint32_t) output);
    }

    // start PWM timer
    SCTIMER_StartTimer(LED_PERIPHERAL, kSCTIMER_Counter_U);
  }
  else
  {
    if (dutyCyclePercent > 100) {
      dutyCyclePercent = 100;
    }

    // stop PWM timer
    SCTIMER_StopTimer(LED_PERIPHERAL, kSCTIMER_Counter_U);

    if (level == kSCTIMER_LowTrue) {
      // enable events
      base->OUT[whichIO].CLR |= (1UL << periodEvent);
      base->OUT[whichIO].SET |= (1UL << pulseEvent);
    }else{
      // enable events
      base->OUT[whichIO].SET |= (1UL << periodEvent);
      base->OUT[whichIO].CLR |= (1UL << pulseEvent);
    }

    // start PWM timer
    SCTIMER_StartTimer(LED_PERIPHERAL, kSCTIMER_Counter_U);

    // update duty cycle
    SCTIMER_UpdatePwmDutycycle(SCT0, output, dutyCyclePercent,  periodEvent);
  }
}

void
BOARD_SetRGB(
  uint8_t red_duty_cycle_percent,
  uint8_t grn_duty_cycle_percent,
  uint8_t blu_duty_cycle_percent
  )
{
  // Control the LEDs
  BOARD_UpdatePwmDutycycle(LED_PERIPHERAL, SCTIMER_OUT_RED_LED,
    RED_LED_TRUE_PWM_LEVEL, red_duty_cycle_percent, LED_pwmEvent[RED_LED_EVENT]);
  BOARD_UpdatePwmDutycycle(LED_PERIPHERAL, SCTIMER_OUT_GRN_LED,
    GRN_LED_TRUE_PWM_LEVEL, grn_duty_cycle_percent, LED_pwmEvent[GRN_LED_EVENT]);
  BOARD_UpdatePwmDutycycle(LED_PERIPHERAL, SCTIMER_OUT_BLU_LED,
    BLU_LED_TRUE_PWM_LEVEL, blu_duty_cycle_percent, LED_pwmEvent[BLU_LED_EVENT]);

  // If all the LEDs are OFF, halt the SCTimer to save power.
  if( red_duty_cycle_percent==0 &&
      grn_duty_cycle_percent==0 &&
      blu_duty_cycle_percent==0 ) {
    // Halt the SCTimer.
    LED_PERIPHERAL->CTRL |= (SCT_CTRL_HALT_L_MASK | SCT_CTRL_HALT_H_MASK);
    // The above code was copied from "fsl_sctimer.c",
    // from the function SCTIMER_Deinit().
    // The SCTIMER_Deinit function can optionally control the clocks,
    // using this snippet of code ensures this function never halts
    // the clocks.
  }else{
    // If the SCT0 peripheral is halted.
    if ( (LED_PERIPHERAL->CTRL & SCT_CTRL_HALT_L_MASK) != 0 &&
         (LED_PERIPHERAL->CTRL & SCT_CTRL_HALT_H_MASK) != 0 ) {
      // Start the SCTimer.
      SCTIMER_Init(LED_PERIPHERAL, &LED_initConfig);
    }
  }
}
#endif

// Convert byte value (0-255) to percent value (0-100)
#define BYTE_TO_PERCENT(X) (((X) * 100)/255)

void
BOARD_SetRGB32(uint32_t rgb_duty_cycle_bytes)
{
  BOARD_SetRGB(
    BYTE_TO_PERCENT((rgb_duty_cycle_bytes & 0x00FF0000) >> 16),
    BYTE_TO_PERCENT((rgb_duty_cycle_bytes & 0x0000FF00) >> 8),
    BYTE_TO_PERCENT((rgb_duty_cycle_bytes & 0x000000FF))
    );
}

void
BOARD_ToggleDebugLED(void) {
  GPIO_PortToggle(DEBUG_LED_GPIO, DEBUG_LED_PORT, DEBUG_LED_PIN_MASK);
}

/******************************************************************************
 * BUTTONS
 *****************************************************************************/
