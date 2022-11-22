/*
 * predict.c
 *
 * Code generation for function 'predict'
 *
 */

/* Include files */
#include "predict.h"
#include "conv2dDirectOptimizedColMajor.h"
#include "rt_nonfinite.h"
#include "sleepstagescorer_internal_types.h"
#include "rt_nonfinite.h"
#include <math.h>

/* Type Definitions */
#ifndef typedef_cell_wrap_7
#define typedef_cell_wrap_7
typedef struct {
  float f1[1250];
} cell_wrap_7;
#endif /* typedef_cell_wrap_7 */

#ifndef typedef_cell_wrap_12
#define typedef_cell_wrap_12
typedef struct {
  float f1[30000];
} cell_wrap_12;
#endif /* typedef_cell_wrap_12 */

#ifndef typedef_cell_wrap_21
#define typedef_cell_wrap_21
typedef struct {
  float f1[1200];
} cell_wrap_21;
#endif /* typedef_cell_wrap_21 */

/* Function Declarations */
static void b_microKernel(int K, const float *A, const float *B, float *C);

static int div_s32_floor(int numerator, int denominator);

static void macroKernel(int M, int K, const float *A, const float *B, float *C);

static void matrixMultiply(int blockSizeM, const float *A, const float *B,
                           float *C);

static void microKernel(int K, const float *A, const float *B, float *C);

/* Function Definitions */
static void b_microKernel(int K, const float *A, const float *B, float *C)
{
  float a;
  float b;
  float c;
  int A_idx;
  int B_idx;
  int k;
  A_idx = 0;
  B_idx = 0;
  c = C[0];
  for (k = 0; k < K; k++) {
    a = A[A_idx];
    b = B[B_idx];
    c += a * b;
    A_idx += 5;
    B_idx++;
  }
  C[0] = c;
}

static int div_s32_floor(int numerator, int denominator)
{
  unsigned int absDenominator;
  unsigned int absNumerator;
  int quotient;
  unsigned int tempAbsQuotient;
  boolean_T quotientNeedsNegation;
  if (denominator == 0) {
    if (numerator >= 0) {
      quotient = MAX_int32_T;
    } else {
      quotient = MIN_int32_T;
    }
  } else {
    if (numerator < 0) {
      absNumerator = ~(unsigned int)numerator + 1U;
    } else {
      absNumerator = (unsigned int)numerator;
    }
    if (denominator < 0) {
      absDenominator = ~(unsigned int)denominator + 1U;
    } else {
      absDenominator = (unsigned int)denominator;
    }
    quotientNeedsNegation = ((numerator < 0) != (denominator < 0));
    tempAbsQuotient = absNumerator / absDenominator;
    if (quotientNeedsNegation) {
      absNumerator %= absDenominator;
      if (absNumerator > 0U) {
        tempAbsQuotient++;
      }
      quotient = -(int)tempAbsQuotient;
    } else {
      quotient = (int)tempAbsQuotient;
    }
  }
  return quotient;
}

static void macroKernel(int M, int K, const float *A, const float *B, float *C)
{
  int A_idx;
  int B_idx;
  int i;
  int j;
  j = 0;
  B_idx = 0;
  while (j <= 0) {
    j = 0;
    i = 0;
    A_idx = 0;
    while (i <= M - 2) {
      microKernel(K, &A[A_idx], &B[B_idx], &C[j]);
      A_idx += 2;
      j += 2;
      i += 2;
    }
    while (i <= M - 1) {
      b_microKernel(K, &A[A_idx], &B[B_idx], &C[j]);
      A_idx++;
      j++;
      i++;
    }
    B_idx += 1200;
    j = 1;
  }
}

static void matrixMultiply(int blockSizeM, const float *A, const float *B,
                           float *C)
{
  int K2;
  int b_i;
  int i;
  int i0;
  int i0_ub;
  int k;
  int k0;
  int k0_ub;
  if (blockSizeM >= 5) {
    blockSizeM = 5;
  } else {
    blockSizeM = (blockSizeM >> 1) << 1;
    if (blockSizeM <= 0) {
      blockSizeM = 1;
    }
  }
  i0_ub = div_s32_floor(4, blockSizeM) + 1;
  k0_ub = div_s32_floor(1199, 128) + 1;
  for (k0 = 1; k0 <= k0_ub; k0++) {
    k = (k0 - 1) * 128;
    if (k > 1072) {
      K2 = 1200 - k;
    } else {
      K2 = 128;
    }
    for (i0 = 1; i0 <= i0_ub; i0++) {
      i = (i0 - 1) * blockSizeM;
      if (i > 5 - blockSizeM) {
        b_i = 5 - i;
      } else {
        b_i = blockSizeM;
      }
      macroKernel(b_i, K2, &A[i + 5 * k], &B[k], &C[i]);
    }
  }
}

static void microKernel(int K, const float *A, const float *B, float *C)
{
  float a;
  float b;
  float b_a;
  float b_c;
  float c;
  int A_idx;
  int B_idx;
  int k;
  A_idx = 0;
  B_idx = 0;
  c = C[0];
  b_c = C[1];
  for (k = 0; k < K; k++) {
    a = A[A_idx];
    b_a = A[A_idx + 1];
    b = B[B_idx];
    c += a * b;
    b_c += b_a * b;
    A_idx += 5;
    B_idx++;
  }
  C[0] = c;
  C[1] = b_c;
}

void DeepLearningNetwork_predict(c_coder_ctarget_DeepLearningNet *obj,
                                 const float varargin_1[1250], float varargout_1[5])
{
  static cell_wrap_12 outT_f3;
  static const float t0_CombinedBeta[24] = {
      -0.054599531F, -0.152810395F,  -0.178983122F, -0.142558962F,
      -0.16565454F,  -0.203764811F,  -0.136154324F, -0.12825036F,
      -0.186976671F, -0.0507784523F, -0.169059485F, -0.123166688F,
      -0.126374632F, -0.19811365F,   -0.171386316F, -0.966424644F,
      -0.185791105F, -0.13746652F,   -0.179107532F, -0.573936582F,
      -0.105081849F, -0.586034536F,  -0.266472638F, -0.265711427F};
  static const float t0_CombinedGamma[24] = {
      96.3293762F, 37.0788574F, 29.9826107F, 44.3544922F, 93.0799484F,
      48.3835754F, 47.2129936F, 84.1442184F, 64.5108109F, 127.305977F,
      130.736298F, 91.6970825F, 50.1239204F, 24.3849201F, 65.878624F,
      88.9982834F, 74.8747787F, 173.616089F, 82.5290222F, 101.560799F,
      39.7115669F, 135.999573F, 65.1216049F, 16.8139782F};
  static const float fv[5] = {0.00511874864F, -0.0870401934F, 0.0237656906F,
                              0.0621848293F, 0.00233152043F};
  cell_wrap_21 outT_f4;
  cell_wrap_7 outT_f1;
  float inputPixel;
  float maxValue;
  int filterWidthIdx;
  int inputWidthIdx;
  int prodOutDimsIdx;
  int vk;
  int widthIdx;
  if (!obj->StateInitialized) {
    obj->StateInitialized = true;
  }
  for (vk = 0; vk < 1250; vk++) {
    outT_f1.f1[vk] = varargin_1[vk] - 0.000351971015F;
  }
  conv2dDirectOptimizedColMajor(outT_f1.f1, outT_f3.f1);
  for (vk = 0; vk < 24; vk++) {
    for (widthIdx = 0; widthIdx < 1250; widthIdx++) {
      inputWidthIdx = widthIdx + 1250 * vk;
      outT_f3.f1[inputWidthIdx] =
          t0_CombinedGamma[vk] * outT_f3.f1[inputWidthIdx] +
          t0_CombinedBeta[vk];
    }
  }
  for (prodOutDimsIdx = 0; prodOutDimsIdx < 1200; prodOutDimsIdx++) {
    vk = prodOutDimsIdx / 50;
    widthIdx = prodOutDimsIdx - vk * 50;
    maxValue = -3.402823466E+38F;
    for (filterWidthIdx = 0; filterWidthIdx < 25; filterWidthIdx++) {
      inputWidthIdx = filterWidthIdx + 25 * widthIdx;
      if ((inputWidthIdx + 1 > 0) && (inputWidthIdx + 1 <= 1250)) {
        inputPixel = outT_f3.f1[inputWidthIdx + 1250 * vk];
        if (rtIsNaNF(inputPixel)) {
          inputPixel = -3.402823466E+38F;
        }
      } else {
        inputPixel = -3.402823466E+38F;
      }
      if ((!(maxValue >= inputPixel)) && (!rtIsNaNF(inputPixel))) {
        maxValue = inputPixel;
      }
    }
    outT_f4.f1[prodOutDimsIdx] = maxValue;
  }
  for (inputWidthIdx = 0; inputWidthIdx < 5; inputWidthIdx++) {
    varargout_1[inputWidthIdx] = fv[inputWidthIdx];
  }
  matrixMultiply(128, &X[0], &outT_f4.f1[0], &varargout_1[0]);
  maxValue = varargout_1[0];
  if ((!(varargout_1[0] >= varargout_1[1])) && (!rtIsNaNF(varargout_1[1]))) {
    maxValue = varargout_1[1];
  }
  if ((!(maxValue >= varargout_1[2])) && (!rtIsNaNF(varargout_1[2]))) {
    maxValue = varargout_1[2];
  }
  if ((!(maxValue >= varargout_1[3])) && (!rtIsNaNF(varargout_1[3]))) {
    maxValue = varargout_1[3];
  }
  if ((!(maxValue >= varargout_1[4])) && (!rtIsNaNF(varargout_1[4]))) {
    maxValue = varargout_1[4];
  }
  for (vk = 0; vk < 5; vk++) {
    varargout_1[vk] = (float)exp(varargout_1[vk] - maxValue);
  }
  maxValue =
      (((varargout_1[0] + varargout_1[1]) + varargout_1[2]) + varargout_1[3]) +
      varargout_1[4];
  for (inputWidthIdx = 0; inputWidthIdx < 5; inputWidthIdx++) {
    varargout_1[inputWidthIdx] /= maxValue;
  }
}

/* End of code generation (predict.c) */
