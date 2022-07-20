/*
 * sleepstagescorer.c
 *
 * Code generation for function 'sleepstagescorer'
 *
 */

/* Include files */
#include "sleepstagescorer.h"
#include "predict.h"
#include "rt_nonfinite.h"
#include "sleepstagescorer_data.h"
#include "sleepstagescorer_initialize.h"
#include "sleepstagescorer_internal_types.h"

/* Variable Definitions */
static c_coder_ctarget_DeepLearningNet cnnnet;

static boolean_T cnnnet_not_empty;

/* Function Definitions */
void sleepstagescorer(float out[5])
{
  if (!isInitialized_sleepstagescorer) {
    sleepstagescorer_initialize();
  }
  /*  sleepstagescorer   Makes prediction on EEG data with CNN network */
  /*  */
  /*    Y = sleepstagescorer() computes predictions on a 10sec epoch of EEG  */
  /*    sleep data using a CNN classifier. Predifined input is a vector with  */
  /*    size [1 1250] (i.e., 10sec of a single channel EEG (fpz) sampled at  */
  /*    125Hz). Y is a vector of scores corresponding to each sleep stage,  */
  /*    respectively (i.e., Wake, REM, N1, N2, N3). */
  /*    For testing, run the following command: "sleepstagescorer()". Further */
  /*    details at: https://www.mathworks.com/help/coder/ug/generate-generic-cc
   */
  /*    -code-for-deep-learning-networks.html  */
  /*    Copyright 2022 The Elemind Technologies, Inc. */
  /*  Input data */
  /*  A persistent object cnnnet is used to load the series network object */
  if (!cnnnet_not_empty) {
    cnnnet.StateInitialized = false;
    cnnnet.matlabCodegenIsDeleted = false;
    cnnnet_not_empty = true;
  }
  /*  Pass in input    */
  DeepLearningNetwork_predict(&cnnnet, out);
}

void sleepstagescorer_free(void)
{
  if (!cnnnet.matlabCodegenIsDeleted) {
    cnnnet.matlabCodegenIsDeleted = true;
  }
}

void sleepstagescorer_init(void)
{
  cnnnet_not_empty = false;
  cnnnet.matlabCodegenIsDeleted = true;
}

/* End of code generation (sleepstagescorer.c) */
