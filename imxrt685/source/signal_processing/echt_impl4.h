/*
 * ECHT_IMPL4.h
 *
 *  Created on: Jan 12, 2019
 *      Author: David Wang
 */

#ifndef _ECHT_COMPONENT4_H_
#define _ECHT_COMPONENT4_H_

extern "C" {
/* Avoid "used but never defined" compile error for linker symbols in
   cmsis_gcc.h. */
#include "arm_math.h"
}
#include "arm_const_structs.h"
#include <algorithm>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <ButterworthBandpass.h>
#include "window.h"
#include "math_util.h"
#include "echt_contol.h"
#include "waveconst.h"
#include "loglevels.h"



template <int MAX_WIN_SIZE, int MAX_FFT_SIZE, int MAX_BB_ORDER, int MAX_BB_NFFT>
class echt_impl4 {
public:
  // CONTROL DATA
  ECHT_Control cntrl;
private:
  const char* TAG = "echt_impl4";    // Logging prefix for this module

  // FILTER
  ButterworthBandpassFreq<float, MAX_BB_ORDER, MAX_BB_NFFT> bpfilter;
  // FFT
  q31_t fft_in[MAX_FFT_SIZE] = {0};
  q31_t fft_out[2*MAX_FFT_SIZE] = {0};
  int32_t echt_coeff[2*MAX_FFT_SIZE] = {0};
  // SAMPLE WINDOW
  Window<q31_t, MAX_WIN_SIZE> window;
  arm_rfft_instance_q31 fftInstance;

public:
  echt_impl4() : window(MAX_FFT_SIZE) {

  }

  virtual ~echt_impl4() {

  }

  uint8_t setCntrl(int fftSize, int filtOrder, float centerFreq, float lowFreqOffset, float highFreqOffset, double sampFreq, int ADCdrop) {

    // only allow FFT SIZES supported by PowerQuad RFFT.
    if(fftSize!=32 && fftSize!=64 && fftSize!=128 && fftSize!=256 && fftSize!=512 && fftSize!=1024 && fftSize!=2048){
        return -1; // error
    }

    // ** Compute some additional internal values for CNTRL.
    // compute other control values based on FFT size
    cntrl.setFFTSize(fftSize, fftSize);

    // ** Continue updating the structures outside of CNTRL.
    // redesign filter
    sampFreq = sampFreq / (ADCdrop+1);
    redesignFilterWithOffsets(filtOrder, centerFreq, lowFreqOffset, highFreqOffset, sampFreq, fftSize);

    // set the size of the sample window
    window.resize(cntrl.winSize);

    return 0; // success
  }

  void addData(int32_t datum) {
    // multiply by fftSize to remove an inherent divide by
    // fftSize built into the powerquad's FFT functionality.
    datum *= cntrl.fftSize;
    // save space for any int32_t to q31_t conversion needed.
    q31_t q_datum = datum;
    // but the datum in the buffer for ecHT processing.
    window.add(q_datum);
  }

  bool isReady(){
    return window.full();
  }

  void computeInstAmpPhase(float& instAmpFromECHT, float& instPhaseFromECHT) {
    // SETUP THE DATA INPUT DATA STRUCTURE
    window.copyto(fft_in);

//    for(int i=0; i<128; i++){
//      fft_in[i] = (1<<24)*sin(i*(2*M_PI)/32);
//    }

//    LOGV("echt_impl", "FFT IN");
//    for(int i=0; i<cntrl.fftSize; i++) {
//      LOGV("echt_impl", "%d", fft_in[i]);
//    }

    // Compute ECHT
    echt(instAmpFromECHT, instPhaseFromECHT);
  }


  void echt(float &instAmp, float &instPhase) {
    int fft_size = cntrl.fftSize;
    int mirror = fft_size/2;


    /* ==== */
    // Serial.print("Data: ");
    // for(int i=0; i<fft_size; i++){
    //   Serial.print(data_in_out[2*i]);
    //   Serial.print("," );
    // }
    // Serial.println();
    /* ==== */


    // GET FREQUENCY COEFFICIENTS
    float* ffcr = bpfilter.getFFCr();
    float* ffci = bpfilter.getFFCi();
    // size of coefficients is: nfft/2+1

    /* ==== */
    // Serial.println("filter coeffs: ");
    // for(int i=0; i<L/2+1; i++){
    //   Serial.print(ffcr[i]);
    //   Serial.print("+");
    //   Serial.print(ffci[i]);
    //   Serial.println(", ");
    // }
    /* ==== */


    // FFT


    // NOT EVERY CALL TO "arm_rfft_fast_init_f32" is valid.
    // The following lines test these function calls with various FFT Sizes.
    // Serial.println(arm_rfft_fast_init_f32(&fftInstance, 32));
    // Serial.println(arm_rfft_fast_init_f32(&fftInstance, 64));
    // Serial.println(arm_rfft_fast_init_f32(&fftInstance, 128));
    // Serial.println(arm_rfft_fast_init_f32(&fftInstance, 256));
    // Serial.println(arm_rfft_fast_init_f32(&fftInstance, 512));
    // Serial.println(arm_rfft_fast_init_f32(&fftInstance, 1024));
    // Serial.println(arm_rfft_fast_init_f32(&fftInstance, 2048));
    // Serial.println(ARM_MATH_ARGUMENT_ERROR);

    // init FFT
    arm_rfft_init_q31(&fftInstance, fft_size, 0, 1);  // COMMENT

    arm_rfft_q31(&fftInstance, fft_in, fft_out);  // COMMENT

//    LOGV("echt_impl", "START COEFFICIENTS");
//    for(int i=0; i<2*fft_size; i++) {
//      LOGV("echt_impl", "%d", fft_out[i]);
//    }

    /* ==== */
    // Serial.println("FFT: ");
    // Serial.print(fft_out[0]/(L>>1));
    // Serial.println("+0.00i, ");
    // for(int i=1; i<L/2; i++){
    //   Serial.print(fft_out[2*i]/(L>>1));
    //   Serial.print("+");
    //   Serial.print(fft_out[2*i+1]/(L>>1));
    //   Serial.println("i,");
    // }
    // Serial.print(fft_out[1]/(L>>1));
    // Serial.println("+0.00i, ");
    /* ==== */

    for(int i=0; i<2*fft_size; i++){
      echt_coeff[i] = fft_out[i]*(1<<31)/(fft_size>>1);
    }

    size_t sindex = 1;

    /* COMPUTE THE INVERSE FFT (IFFT) FOR THE SAMPLE INDEX (sindex) only */
    /* Also, apply the frequency domain filter coefficients */
    /* The IFFT is only computed for the POSITIVE coefficients */
    /* SINE AND COSINE are implemented as lookups in a lookup table*/
    uint16_t stepsize = NWAVE / fft_size * sindex; // if FFT_SIZE is less than 2048, this number will always be less than 2048
    uint16_t cosstep = NQUAT;
    uint16_t sinstep = 0;
    // multiply by the amplitude of the Sinewave, because it is used in the computation of other coefficients.
    int cr = ((echt_coeff[0] >> 1) << 12) * ffcr[0]; // divide by 2
    int ci = 0;
    for (int i = 1; i < mirror; i++) {
      cosstep = (cosstep + stepsize) % NWAVE;
      sinstep = (sinstep + stepsize) % NWAVE;
      int fftr = echt_coeff[2*i] * Sinewave[cosstep] + echt_coeff[2*i+1] * Sinewave[sinstep];
      int ffti = echt_coeff[2*i+1] * Sinewave[cosstep] - echt_coeff[2*i] * Sinewave[sinstep];
      cr += fftr * ffcr[i] - ffti * ffci[i];
      ci += ffti * ffcr[i] + fftr * ffci[i];
    }
    int nyquist = (echt_coeff[1] >> 1); // divide by 2
    int fftr = nyquist * (Sinewave[(1024 * sindex + NQUAT) % NWAVE]);
    int ffti = -nyquist * (Sinewave[(1024 * sindex) % NWAVE]);
    cr += fftr * ffcr[mirror] - ffti * ffci[mirror];
    ci += ffti * ffcr[mirror] + fftr * ffci[mirror];

    // Remove the 4096 multiple introduced by the Sinewave lookup table.
    cr = (cr < 0) ? ((cr >> 12) + 1) : (cr >> 12);
    ci = (ci < 0) ? ((ci >> 12) + 1) : (ci >> 12);


    // Compute Instantaneous Amp and Phase
    /* ==== */
    // Serial.println("Inv FFT: ");
    // Serial.print(cr);
    // Serial.print("+");
    // Serial.print(ci);
    // Serial.println("i");
    /* ==== */

    instAmp = sqrt(cr * cr + ci * ci);
    instPhase = wrapTo2Pi(atan2_approximation2(ci, cr));

  }

  void redesignFilter(int order, double lowFreq, double highFreq, double sampFreq, int fftSize) {
    bpfilter.design(order, lowFreq, highFreq, sampFreq);
    bpfilter.designffc(fftSize);
  }

  void redesignFilterWithOffsets(int order, double centerFreq, double lowFreqOffset, double highFreqOffset, double sampFreq, int fftSize) {
    if (lowFreqOffset > highFreqOffset) {
      // swap low and high frequencies, if they were entered backwards.
      double temp = lowFreqOffset;
      lowFreqOffset = highFreqOffset;
      highFreqOffset = temp;
    }
    bpfilter.design(order, std::max(0.01, centerFreq + lowFreqOffset), std::max(0.01, centerFreq + highFreqOffset), sampFreq);
    bpfilter.designffc(fftSize);
  }

};

#endif /* _ECHT_COMPONENT4_H_ */
