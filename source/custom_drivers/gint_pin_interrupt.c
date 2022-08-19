/*
 * gint_pin_interrupt.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jun, 2020
 * Author:  David Wang
 *
 * Description: Wraps the port-level interrupts from the GINT peripheral
 *              with pin-level interrupts. Supports registration of
 *              ISRs, per-pin on all GPIO pins. ISRs are edge triggered
 *              on rising, falling, or change (rising or falling).
 */
#include <stdbool.h>

#include "gint_pin_interrupt.h"
#include "loglevels.h"

// Logging prefix for this module
static const char *TAG __attribute__ ((unused)) = "gint_pint";


void gint_pint_erase_config(gint_pint_param_t *param){
  GINT_Type *gint_base = param->gint_base;
  // erase the polarity and enabled masks for all ports.
  GINT_ConfigPins(gint_base, kGINT_Port0, 0, 0);
  GINT_ConfigPins(gint_base, kGINT_Port1, 0, 0);
#if defined(FSL_FEATURE_GINT_PORT_COUNT) && (FSL_FEATURE_GINT_PORT_COUNT > 2U)
  GINT_ConfigPins(gint_base, kGINT_Port2, 0, 0);
#endif
#if defined(FSL_FEATURE_GINT_PORT_COUNT) && (FSL_FEATURE_GINT_PORT_COUNT > 3U)
  GINT_ConfigPins(gint_base, kGINT_Port3, 0, 0);
#endif
#if defined(FSL_FEATURE_GINT_PORT_COUNT) && (FSL_FEATURE_GINT_PORT_COUNT > 4U)
  GINT_ConfigPins(gint_base, kGINT_Port4, 0, 0);
#endif
#if defined(FSL_FEATURE_GINT_PORT_COUNT) && (FSL_FEATURE_GINT_PORT_COUNT > 5U)
  GINT_ConfigPins(gint_base, kGINT_Port5, 0, 0);
#endif
#if defined(FSL_FEATURE_GINT_PORT_COUNT) && (FSL_FEATURE_GINT_PORT_COUNT > 6U)
  GINT_ConfigPins(gint_base, kGINT_Port6, 0, 0);
#endif
#if defined(FSL_FEATURE_GINT_PORT_COUNT) && (FSL_FEATURE_GINT_PORT_COUNT > 7U)
  GINT_ConfigPins(gint_base, kGINT_Port7, 0, 0);
#endif
}

void gint_pint_config(gint_pint_param_t *param, bool erase_prev_gint_config)
{
  GPIO_Type *gpio_base = param->gpio_base;
  GINT_Type *gint_base = param->gint_base;

  // Erase polarity and enable mask.
  if (erase_prev_gint_config) {
    gint_pint_erase_config(param);
  }

  // Compute & Set polarity and enable mask.
  for ( int p = 0; p<param->num_monitored_pins; p++) {
    // get a monitored pin.
    gint_pint_pin_param_t *mpin = &(param->monitored_pins[p]);
    // read the current pin value
    uint32_t curr_val = GPIO_PinRead(gpio_base, mpin->port, mpin->pin);
    // set enabled mask
    gint_base->PORT_ENA[mpin->port] |= (1 << mpin->pin);
    // set polarity mask
    if(curr_val) {
      gint_base->PORT_POL[mpin->port] &= ~(1 << mpin->pin);
    }
    else {
      gint_base->PORT_POL[mpin->port] |= (1 << mpin->pin);
    }
    // record value of the pin
    mpin->prev_val = curr_val;
  }

  /* Set GINT control registers and the name of callback function */
  GINT_SetCtrl(gint_base, kGINT_CombineOr, kGINT_TrigEdge, param->gint_irq_callback);
  /* Interrupt vector GINT0_IRQn priority settings in the NVIC */
  NVIC_SetPriority(param->gint_irq_n, param->gint_irq_priority);
  /* Enable callback for GINT */
  GINT_EnableCallback(gint_base);
}

void gint_pint_update(gint_pint_param_t *param, bool clear_gint_interrupt)
{
  GPIO_Type *gpio_base = param->gpio_base;
  GINT_Type *gint_base = param->gint_base;

  // clear interrupt status
  if (clear_gint_interrupt) {
    GINT_ClrStatus(gint_base);
  }

  // loop over the array of monitored pins
  for (int p = 0; p<param->num_monitored_pins; p++) {
    // get a monitored pin.
    gint_pint_pin_param_t *mpin = &(param->monitored_pins[p]);
    // read the current pin value
    uint32_t curr_val = GPIO_PinRead(gpio_base, mpin->port, mpin->pin);
    // check if the pin value was rising or falling
    if (mpin->prev_val != curr_val) {
      // update the polarity mask
      if(curr_val) {
        // detected rising edge
        // change polarity to detect next falling edge
        gint_base->PORT_POL[mpin->port] &= ~(1 << mpin->pin);
        // callback
        if(mpin->mode == gint_pint_mode_rising ||
           mpin->mode == gint_pint_mode_change ){
          mpin->callback(gpio_base, mpin->port, mpin->pin, gint_pint_mode_rising);
        }
      }
      else {
        // detected falling edge
        // change polarity to detect next rising edge
        gint_base->PORT_POL[mpin->port] |= (1 << mpin->pin);
        // callback
        if(mpin->mode == gint_pint_mode_falling ||
           mpin->mode == gint_pint_mode_change ){
          mpin->callback(gpio_base, mpin->port, mpin->pin, gint_pint_mode_falling);
        }
      }
      // save off the new pin value
      mpin->prev_val = curr_val;
    }
  }
}
