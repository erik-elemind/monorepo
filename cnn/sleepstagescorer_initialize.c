/*
 * sleepstagescorer_initialize.c
 *
 * Code generation for function 'sleepstagescorer_initialize'
 *
 */

/* Include files */
#include "sleepstagescorer_initialize.h"
#include "rt_nonfinite.h"
#include "sleepstagescorer.h"
#include "sleepstagescorer_data.h"

/* Function Definitions */
void sleepstagescorer_initialize(void)
{
  rt_InitInfAndNaN();
  sleepstagescorer_init();
  isInitialized_sleepstagescorer = true;
}

/* End of code generation (sleepstagescorer_initialize.c) */
