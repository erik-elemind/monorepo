/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
#include "main.h"
#include "rt_nonfinite.h"
#include "sleepstagescorer.h"
#include "sleepstagescorer_terminate.h"
#include <stdio.h>

/* Function Declarations */
static void main_sleepstagescorer(void);

/* Function Definitions */
static void main_sleepstagescorer(void)
{
  float out[5] = {0};
  char outbuff[50] = {0};

  /* Call the entry-point 'sleepstagescorer'. */
  sleepstagescorer(out);

  for (int i = 0;i < (sizeof (out) /sizeof (out[0])); i++) {
	  sprintf(outbuff, "out[%d]: %.11f\r\n",i, out[i]);
	  PRINTF(outbuff);
  }
}

int main(void)
{
    char ch;

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    PRINTF("Start Machine Learning\r\n");

    // Random classifier
      /* The initialize function is being called automatically from your entry-point
       * function. So, a call to initialize is not included here. */
      /* Invoke the entry-point functions.
    You can call entry-point functions multiple times. */
      main_sleepstagescorer();
      /* Terminate the application.
    You do not need to do this more than one time. */
      sleepstagescorer_terminate();

      PRINTF("Done!\r\n");

      while(1)
      {
    	  ch = GETCHAR();
    	  PUTCHAR(ch);
      }

      return 0;
}
