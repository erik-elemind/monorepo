/*
 * ECHTImpl5.h
 *
 *  Created on: Jan 12, 2019
 *      Author: David Wang
 */

#ifndef _ECHT_IMPL5_H_
#define _ECHT_IMPL5_H_

//#include <Arduino.h>
/* http://coolarduino.wordpress.com/2014/03/11/fft-library-for-arduino-due-sam3x-cpu/ */

#if (defined(ENABLE_ECHT_INTERFACE) && (ENABLE_ECHT_INTERFACE > 0U))
#include "Interface/Stream/StreamCommand.h"
#include "Interface/Stream/StreamCodes.h"
#include "Interface/InterfaceAdapter.h"
#include "CustomCommandCodes.h"
#endif

//#include "SignalProcessing/ButterworthBandpass.h"
//#include "SignalProcessing/window.h"
//#include "SignalProcessing/waveconst.h"
//#include "Utils/math_util.h"
#include "ButterworthBandpass.h"
#include "window.h"
#include "waveconst.h"
#include "math_util.h"
#include "ECHTSettings.h"
#include "ECHTComponent.h"
#include "arm_math.h"
#include "arm_const_structs.h"

#include "min_max.h"
#include "portmacro.h"
#include "powerquad_helper.h"


#ifndef ENABLE_IN_ECHT_TIMING_METRICS
#define ENABLE_IN_ECHT_TIMING_METRICS 0U
#endif 

#ifndef ENABLE_ECHT_OLD_MIN_MAX
#define ENABLE_ECHT_OLD_MIN_MAX 0U
#endif 

template <int MAX_WIN_SIZE, int MAX_FFT_SIZE, int MAX_BB_ORDER, int MAX_BB_NFFT>
class ECHTImpl5 : public ECHTComponent {

private:
  // FILTER
  ButterworthBandpassFreq<float, MAX_BB_ORDER, MAX_BB_NFFT> bpfilter;
  // FFT
  int32_t data_in[MAX_FFT_SIZE] __attribute__ ((aligned (4)));
  int32_t data_out[2*MAX_FFT_SIZE]  __attribute__ ((aligned (4)));
  // SAMPLE WINDOW
//  float win_mag_array[MAX_WIN_SIZE];
  float win_mag_array_copy[MAX_WIN_SIZE];
//  Window<float, float, float> win_mag;
  Window<float, MAX_WIN_SIZE> win_mag;
  arm_rfft_instance_q31 fftInstance;
  
  MovingMin<float,MAX_WIN_SIZE> moving_min;
  MovingMax<float,MAX_WIN_SIZE> moving_max;

protected:
  static const size_t num_fft_sizes;
  static const size_t fft_sizes_array[];

  virtual void initStreamCommands() override {
    ECHTComponent::initStreamCommands();
  }

  virtual void initCommands() override {
    ECHTComponent::initCommands();
  }

  /**************************************/
  /* FILTER MANIPULATION FUNCTIONS      */

  virtual void designFilter(int order, double centerFreq, double lowFreq, double highFreq, double sampFreq, int fftSize, bool resetCache) override {
    bpfilter.design(order, lowFreq, highFreq, sampFreq, resetCache);
    bpfilter.designffc(fftSize);
  }

public:

  /**************************************/
  /* CONSTRUCTOR/DESTRUCTOR             */

#if (defined(ENABLE_ECHT_INTERFACE) && (ENABLE_ECHT_INTERFACE > 0U))
  ECHTImpl5(StreamCommandArray* strCmdArr, InterfaceAdapter* userITF) : 
    ECHTComponent(strCmdArr, userITF),
    win_mag(win_mag_array, MAX_WIN_SIZE),
    moving_min(1), moving_max(1)
  {
  }
#else
  ECHTImpl5() :
    win_mag(MAX_WIN_SIZE),
    moving_min(1), moving_max(1)
  {
  }
#endif

  virtual ~ECHTImpl5() {
  }

  virtual void init() override {
    initStreamCommands();
    initCommands();
    applyCntrl(true);
  }

  /**************************************/
  /* CONTROL AND STREAMING FUNCTIONS    */

  virtual void applyCntrl(bool force) override {
    ECHTComponent::applyCntrl(force);

    // set the size of the sample window
    if (force || (size_t)pendSet.winSize != win_mag.getSize()) {
      win_mag.resize(pendSet.winSize);
    }

    // set the moving min and max window sizes
    moving_min.setWindowSize(pendSet.winSize);
    moving_max.setWindowSize(pendSet.winSize);
  }

  /**************************************/
  /* COMPONENT FUNCTIONS                */

  virtual bool isReady() override {
    return win_mag.full();
  }

  /**************************************/
  /* PARAMETER MANIPULATION FUNCTIONS    */

  virtual Array<size_t> getValidFFTSizes() override {
    Array<size_t> fft_sizes((size_t*)fft_sizes_array, num_fft_sizes);
    return fft_sizes;
  }

  /**************************************/
  /* DATA MANIPULATION FUNCTIONS        */

  virtual void addData(float datum) override {
    win_mag.add(datum);
#if !(defined(ENABLE_ECHT_OLD_MIN_MAX) && (ENABLE_ECHT_OLD_MIN_MAX > 0U))
    win_min_mag_ = moving_min.getMin(datum);
    win_max_mag_ = moving_max.getMax(datum);
#endif
  }

  virtual float getAverage() override {
    // average is unsupported
    return 0;
  }

  /**************************************/
  /* ECHT FUNCTIONS                     */

  virtual void computeInstAmpPhase(float& eeg_now, float& inst_amp, float& inst_phs) override {
    // Find Average of Data
//    noInterrupts();
    // float avg = win_mag.average();
    win_mag.copyto(win_mag_array_copy);
//    interrupts();

    // save off eeg_now
    eeg_now = win_mag_array_copy[currSet.sampleSize-1];

    // Find the min/max range for scaling
    float dynamic_scale = 1;
    if (currSet.inputScale < 0) {
#if (defined(ENABLE_ECHT_OLD_MIN_MAX) && (ENABLE_ECHT_OLD_MIN_MAX > 0U))
      float tmp_min_mag = win_mag_array_copy[currSet.zeroSize];
      float tmp_max_mag = win_mag_array_copy[currSet.zeroSize];
      for ( uint16_t i = 0; i < currSet.sampleSize; i++)
      {
        float val = win_mag_array_copy[i + currSet.zeroSize]; // SAMPLE_SIZE
        tmp_min_mag = min(tmp_min_mag, val);
        tmp_max_mag = max(tmp_max_mag, val);
      }
      win_min_mag_ = tmp_min_mag;
      win_max_mag_ = tmp_max_mag;
#endif

      dynamic_scale = 4096.0f / (win_max_mag_ - win_min_mag_);

      // Remove DC offset, and 
      // Add a scaling to reduce integer round-off.
      for ( uint16_t i = 0; i < currSet.sampleSize; i++)
      {
        float val = win_mag_array_copy[i + currSet.zeroSize]; // SAMPLE_SIZE
        data_in[i] = val * dynamic_scale;

        if (i == currSet.sampleSize - 1) {
          insig_nodc_ = data_in[i];
        }
      }

    } else {
      // Remove DC offset, and 
      // Add a scaling to reduce integer round-off.
      for ( uint16_t i = 0; i < currSet.sampleSize; i++)
      {
        float val = win_mag_array_copy[i + currSet.zeroSize]; // SAMPLE_SIZE
        data_in[i] = val * currSet.inputScale;
        // we do not subtract the average as the moving window average with floating point numbers will be inaccurate

        if (i == currSet.sampleSize - 1) {
          insig_nodc_ = data_in[i];
        }
      }

    }

    // Fill the remaining data with zeros
    for (uint16_t i = 0; i < currSet.zeroSize; i++) {
      data_in[i + currSet.sampleSize] = 0;
    }

    // Compute ECHT
    echt(currSet.sampleSize - 1, inst_amp_, inst_phs_);

    // Remove the scaling
    if (currSet.inputScale < 0) {
      inst_amp_ /= dynamic_scale;
    } else {
      inst_amp_ /= currSet.inputScale;
    }

    inst_amp = inst_amp_;
    inst_phs = inst_phs_;

    // Generate filtered signal
    insig_filt_ = inst_amp_ * cos(inst_phs_);
  }
  
  virtual void echt(int sindex, float &instAmp, float &instPhase) override {
    int fft_size = currSet.fftSize;
    int mirror = fft_size/2;

    // only allow FFT SIZES supported by RFFT.
    if(!isValidFFTSize(fft_size)){
        instAmp = 0;
        instPhase = 0;
        return;
    }

    // SETUP THE DATA INPUT DATA STRUCTURE
    // int32_t data_in[fft_size] __attribute__ ((aligned (4)));
    // int32_t data_out[fft_size*2] __attribute__ ((aligned (4)));
    // for(uint16_t j=0; j < fft_size; j++){
    //   data_in[j] = data_in[j];
    // }

    /* ==== */
    // Serial.print("Data: ");
    // for(int i=0; i<fft_size; i++){
    //   Serial.print(data_in[i]);
    //   Serial.print("," );
    // }
    // Serial.println();
    /* ==== */

#if (defined(ENABLE_IN_ECHT_TIMING_METRICS) && (ENABLE_IN_ECHT_TIMING_METRICS > 0U))
	  uint32_t fft_start_time_us = micros();
#endif

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
    arm_rfft_init_q31(&fftInstance, fft_size, 0, 1);  // COMMENT
    pqhelper_arm_rfft_q31(&fftInstance, data_in, data_out);  // COMMENT  

#if (defined(ENABLE_IN_ECHT_TIMING_METRICS) && (ENABLE_IN_ECHT_TIMING_METRICS > 0U))
    uint32_t fft_delta_us = micros() -  fft_start_time_us;
    Serial.print("FFT delta_us: ");
    Serial.println(fft_delta_us);
#endif

    /* ==== */
    // Serial.println("FFT: ");
    // Serial.print(data_out[0]/(L>>1));
    // Serial.println("+0.00i, ");
    // for(int i=1; i<L/2; i++){
    //   Serial.print(data_out[2*i]/(L>>1));
    //   Serial.print("+");
    //   Serial.print(data_out[2*i+1]/(L>>1));
    //   Serial.println("i,");
    // }
    // Serial.print(data_out[1]/(L>>1));
    // Serial.println("+0.00i, ");
    /* ==== */

    // // Hilbert Coeff Manipulation and Filtering
    // for(uint32_t i = 0; i < L; i++){
    //   if (i == 0){
    //     complex_mult(data_out[0], 0, ffcr[i], ffci[i], data_out[2*i], data_out[2*i+1]);
    //   }else if (i == L/2) {
    //     // case for 0 and nyquist frequencies.
    //     complex_mult(data_out[1], 0, ffcr[i], ffci[i], data_out[2*i], data_out[2*i+1]);
    //   } else if(i < L/2) {
    //     // case for positive frequencies.
    //     complex_mult(2*data_out[2*i], 2*data_out[2*i+1], ffcr[i], ffci[i], data_out[2*i], data_out[2*i+1]);
    //   } else {
    //     // case for negative frequencies.
    //     data_out[2*i] = 0;
    //     data_out[2*i+1] = 0;
    //   }
    // }

    // for(int i=0; i<2*fft_size; i++){
    //   Serial.println(data_out[i]);
    // }

    sindex = fft_size - sindex;

    /* COMPUTE THE INVERSE FFT (IFFT) FOR THE SAMPLE INDEX (sindex) only */
    /* Also, apply the frequency domain filter coefficients */
    /* The IFFT is only computed for the POSITIVE coefficients */
    /* SINE AND COSINE are implemented as lookups in a lookup table*/
    uint16_t stepsize = NWAVE / fft_size * sindex; // if FFT_SIZE is less than 2048, this number will always be less than 2048
    uint16_t cosstep = NQUAT;
    uint16_t sinstep = 0;
    // multiply by the amplitude of the Sinewave, because it is used in the computation of other coefficients.
    int cr = ((data_out[0] >> 1) << 12) * ffcr[0]; // divide by 2
    int ci = 0;
    for (int i = 1; i < mirror; i++) {
      cosstep = (cosstep + stepsize) % NWAVE;
      sinstep = (sinstep + stepsize) % NWAVE;
      int fftr = data_out[2*i] * Sinewave[cosstep] + data_out[2*i+1] * Sinewave[sinstep];
      int ffti = data_out[2*i+1] * Sinewave[cosstep] - data_out[2*i] * Sinewave[sinstep];
      cr += fftr * ffcr[i] - ffti * ffci[i];
      ci += ffti * ffcr[i] + fftr * ffci[i];
    }
    int nyquist = (data_out[1] >> 1); // divide by 2
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

    instAmp = 2*sqrt(cr * cr + ci * ci);
    instPhase = wrapTo2Pi(atan2_approximation2(ci, cr));
  }

};

// TODO: Check this range of FFT Size values. More values may be permitted
template <int MAX_WIN_SIZE, int MAX_FFT_SIZE, int MAX_BB_ORDER, int MAX_BB_NFFT>
const size_t ECHTImpl5<MAX_WIN_SIZE,MAX_FFT_SIZE,MAX_BB_ORDER,MAX_BB_NFFT>::num_fft_sizes = 7;

template <int MAX_WIN_SIZE, int MAX_FFT_SIZE, int MAX_BB_ORDER, int MAX_BB_NFFT>
const size_t ECHTImpl5<MAX_WIN_SIZE,MAX_FFT_SIZE,MAX_BB_ORDER,MAX_BB_NFFT>::fft_sizes_array[] = {32,64,128,256,512,1024,2048,4096,8192};

#endif /* _ECHT_IMPL5_H_ */
