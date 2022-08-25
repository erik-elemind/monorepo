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

#ifndef GINT_PIN_INTERRUPT_H_INC
#define GINT_PIN_INTERRUPT_H_INC

#include <stdbool.h>
#include <stdint.h>

#include "fsl_gpio.h"


// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif


typedef enum gint_pint_mode
{
  gint_pint_mode_rising,
  gint_pint_mode_falling,
  gint_pint_mode_change
} gint_pint_mode;

/*! @brief GINT PINT Callback function. */
typedef void (*gint_pint_cb_t)(GPIO_Type *base, uint32_t port, uint32_t pin, gint_pint_mode mode);

typedef struct _gint_pint_pin_param_t
{
  uint8_t         port;
  uint8_t         pin;
  gint_pint_mode  mode;
  gint_pint_cb_t  callback;
  uint8_t         prev_val;
} gint_pint_pin_param_t;

typedef struct _gint_pint_param_t
{
  GPIO_Type              *gpio_base;  // GPIO
//  GINT_Type              *gint_base;  // GINT0 or GINT1
//  gint_cb_t              gint_irq_callback;
//  IRQn_Type              gint_irq_n;
  uint32_t               gint_irq_priority;
  gint_pint_pin_param_t  *monitored_pins;
  uint8_t                num_monitored_pins;
} gint_pint_param_t;


void gint_pint_erase_config(gint_pint_param_t *param);
void gint_pint_config(gint_pint_param_t *param, bool erase_prev_gint_config);
void gint_pint_update(gint_pint_param_t *param, bool clear_gint_interrupt);



// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif //GINT_PIN_INTERRUPT_H_INC
