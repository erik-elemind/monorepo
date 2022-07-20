/*
 * sleepstagescorer_terminate.c
 *
 * Code generation for function 'sleepstagescorer_terminate'
 *
 */

/* Include files */
#include "sleepstagescorer_terminate.h"
#include "rt_nonfinite.h"
#include "sleepstagescorer.h"
#include "sleepstagescorer_data.h"

/* Function Definitions */
void sleepstagescorer_terminate(void)
{
  sleepstagescorer_free();
  isInitialized_sleepstagescorer = false;
}

/* End of code generation (sleepstagescorer_terminate.c) */
