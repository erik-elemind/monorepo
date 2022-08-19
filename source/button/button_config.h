/*
 * custom_button.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jun, 2020
 * Author:  David Wang
 *
 * Description: Provides implementation-specific initialization for
 *              the generic button task (button.h/c).
 */

#ifndef BUTTON_CONFIG_H_INC
#define BUTTON_CONFIG_H_INC

#include "gint_pin_interrupt.h"
#include "button.h"

#ifdef __cplusplus
extern "C" {
#endif

button_params_t* button_pretask_init();

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_CONFIG_H_INC */
