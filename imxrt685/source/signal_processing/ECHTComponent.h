/*
 * ECHTComponent.h
 *
 *  Created on: Oct 3, 2020
 *      Author: David Wang
 */

#ifndef _ECHT_COMPONENT_H_
#define _ECHT_COMPONENT_H_


#ifndef ENABLE_ECHT_INTERFACE
#define ENABLE_ECHT_INTERFACE 0U
#endif

//#include <Arduino.h>
/* http://coolarduino.wordpress.com/2014/03/11/fft-library-for-arduino-due-sam3x-cpu/ */

#if (defined(ENABLE_ECHT_INTERFACE) && (ENABLE_ECHT_INTERFACE > 0U))
#include "Interface/Stream/StreamCommand.h"
#include "Interface/Stream/StreamCodes.h"
#include "Interface/InterfaceAdapter.h"
#include "CustomCommandCodes.h"
#endif

//#include "Utils/math_util.h"
#include "math_util.h"
#include "ECHTSettings.h"
// #include "arm_math.h"
// #include "arm_const_structs.h"


// #define ENABLE_IN_ECHT_TIMING_METRICS

template<class T>
struct Array {
  const T* const sizes;
  const size_t num;
  Array(T* fft_sizes, size_t num_fft_sizes) : 
  sizes(fft_sizes), num(num_fft_sizes){
  }
};

class ECHTComponent {
private:

public:
  // CONTROL DATA
  ECHTSettings pendSet;
  ECHTSettings currSet;

protected:
  // SIGNAL DATA
  float win_max_mag_;
  float win_min_mag_;
  float insig_nodc_;
  float insig_filt_;
  float inst_amp_;
  float inst_phs_;

  // STREAM COMMANDS
#if (defined(ENABLE_ECHT_INTERFACE) && (ENABLE_ECHT_INTERFACE > 0U))
  StreamCommandArray* strCmdArr_;
  InterfaceAdapter* userITF_;
#endif

#if (defined(ENABLE_ECHT_INTERFACE) && (ENABLE_ECHT_INTERFACE > 0U))
  virtual void initStreamCommands(){
    strCmdArr_->set(SC_INST_AMP, (char*)"iamp", [this](Writer &w){
      w.writeFLOAT(inst_amp_);
    });
    strCmdArr_->set(SC_INST_PHASE, (char*)"iphs", [this](Writer &w){
      w.writeFLOAT(inst_phs_);
    });
    strCmdArr_->set(SC_IN_SIGNAL_NODC, (char*)"innodc", [this](Writer &w){
      w.writeFLOAT(insig_nodc_);
    });
    strCmdArr_->set(SC_IN_SIGNAL_NODC_FILT, (char*)"infilt", [this](Writer &w){
      w.writeFLOAT(insig_filt_);
    });
    strCmdArr_->set(SC_FFT_BIN_FREQ_WIDTH, (char*)"fftbin", [this](Writer &w){
      w.writeFLOAT( getFFTBinWidth_Hz() );
    });
    strCmdArr_->set(SC_WINDOW_DUR, (char*)"windur", [this](Writer &w){
      w.writeFLOAT( getSampleWindowDur_Sec() );
    });
  }
#else
  virtual void initStreamCommands(){}
#endif

  // MESSAGE PROCESSING
#if (defined(ENABLE_ECHT_INTERFACE) && (ENABLE_ECHT_INTERFACE > 0U))
  virtual void initCommands(){
    userITF_->addHandler(CC_INPUT_SCALING,[this](Reader *r){
      float inputScale = r->readFLOAT().getValue();
      this->pendSet.setInputScale(inputScale);
    });
    userITF_->addHandler(CC_COMPUTE_FILTER_WITH_OFFSET,[this](Reader *r){
      int order = r->readINT().getValue();
      float lowFreq = r->readFLOAT().getValue();
      float highFreq = r->readFLOAT().getValue();
      float centerFreq = r->readFLOAT().getValue();
      lowFreq  -= centerFreq;
      highFreq += centerFreq;
      this->pendSet.setFilterParams(order, centerFreq, lowFreq, highFreq);
    });
    userITF_->addHandler(CC_COMPUTE_FILTER_WITH_LOW_HIGH,[this](Reader *r){
      int order = r->readINT().getValue();
      float lowFreq = r->readFLOAT().getValue();
      float highFreq = r->readFLOAT().getValue();
      float centerFreq = (this->pendSet.lowFreq + this->pendSet.highFreq) / 2.0;
      this->pendSet.setFilterParams(order, centerFreq, lowFreq, highFreq);
    });
    userITF_->addHandler(CC_FFT_SIZE,[this](Reader *r){
      int fftSize = r->readINT().getValue();
      if( isValidFFTSize(fftSize) ){
        this->pendSet.setFFTSize(fftSize, fftSize);
      }
    });
  }
#else
  virtual void initCommands(){}
#endif

  inline void resetRunningData(){
    win_max_mag_ = 0;
    win_min_mag_ = 0;
    insig_nodc_ = 0;
    insig_filt_ = 0;
    inst_amp_ = 0;
    inst_phs_ = 0;
  }

  /**************************************/
  /* FILTER MANIPULATION FUNCTIONS      */

  virtual void designFilter(int order, double centerFreq, double lowFreq, double highFreq, double sampFreq, int fftSize, bool resetCache) = 0;

public:

  /**************************************/
  /* CONSTRUCTOR/DESTRUCTOR             */

#if (defined(ENABLE_ECHT_INTERFACE) && (ENABLE_ECHT_INTERFACE > 0U))
  ECHTComponent(StreamCommandArray* strCmdArr, InterfaceAdapter* userITF) : 
    strCmdArr_(strCmdArr), userITF_(userITF)
  {
    resetRunningData();
  }
#else
  ECHTComponent(){
    resetRunningData();
  }
#endif

  ~ECHTComponent() {
  }

  virtual void init() = 0;

  /**************************************/
  /* CONTROL AND STREAMING FUNCTIONS    */

  virtual void setCntrl(int fftSize, int filtOrder, float centerFreq, float lowFreq, float highFreq, float inputScale, double sampFreq) {
    setFFTSize(fftSize, fftSize);
    setFilterParams(filtOrder, centerFreq, lowFreq, highFreq);
    setInputScale(inputScale);
    setSampleFreq(sampFreq);

    // apply the settings
    applyCntrl(true);
  }

  virtual void applyCntrl(bool force) {
    // ** Compute some additional internal values for CNTRL.
    // compute other control values based on FFT size
    if (force || pendSet.fftSize != currSet.fftSize) {
      pendSet.setFFTSize(pendSet.fftSize, pendSet.fftSize);
    }

    // ** Continue updating the structures outside of CNTRL.
    // redesign filter
    if (force || 
        pendSet.filtOrder != currSet.filtOrder ||
        pendSet.centerFreq != currSet.centerFreq ||
        pendSet.lowFreq != currSet.lowFreq ||
        pendSet.highFreq != currSet.highFreq ||
        pendSet.sampFreq != currSet.sampFreq ||
        pendSet.fftSize != currSet.fftSize) {
        // Serial.print("filtOrder "); Serial.println(pendCntrl.filtOrder);
        // Serial.print("centerFreq "); Serial.println(pendCntrl.centerFreq);
        // Serial.print("lowFreq "); Serial.println(pendCntrl.lowFreq);
        // Serial.print("highFreq "); Serial.println(pendCntrl.highFreq);
        // Serial.print("sampFreq "); Serial.println(sampFreq);
        // Serial.print("fftSize"); Serial.println(pendCntrl.fftSize);
      designFilter(pendSet.filtOrder, pendSet.centerFreq, pendSet.lowFreq, pendSet.highFreq, pendSet.sampFreq, pendSet.fftSize, false);
    }

    currSet = pendSet;
  }

  /**************************************/
  /* COMPONENT FUNCTIONS                */

  virtual bool isReady() = 0;

  /**************************************/
  /* PARAMETER MANIPULATION FUNCTIONS   */

  virtual Array<size_t> getValidFFTSizes() = 0;

  virtual bool isValidFFTSize(size_t fftSize) {
    Array<size_t> a = getValidFFTSizes();
    for(size_t i=0; i<a.num; i++){
      size_t validFFTSize = a.sizes[i];
      if(fftSize == validFFTSize){
        return true;
      }
    }
    return false;
  }

  inline float getFFTBinWidth_Hz() {
    return (this->currSet.sampFreq) / (this->currSet.fftSize / 2.0);
  }

  inline float getSampleWindowDur_Sec(){
    return this->currSet.fftSize * 1.0f / this->currSet.sampFreq;
  }

  void setFFTSize(int fft_size, int sample_size) {
    pendSet.setFFTSize(fft_size,fft_size);
  }

  void setFilterParams(int order, float centerFreq, float lowFreq, float highFreq){
    pendSet.setFilterParams(order, centerFreq, lowFreq, highFreq);
  }

  inline void setInputScale(float inputScale){
    pendSet.setInputScale(inputScale);
  }

  void setSampleFreq(double sampFreq){
    pendSet.setSampleFreq(sampFreq);
  }

  /**************************************/
  /* DATA MANIPULATION FUNCTIONS        */

  virtual void addData(float datum) = 0;
  virtual float getAverage() = 0;

  /**************************************/
  /* ECHT FUNCTIONS                     */

  virtual void computeInstAmpPhase(float& eeg_now, float& inst_amp, float& inst_phs) = 0;
  virtual void echt(int sindex, float &instAmp, float &instPhase) = 0;
};


#endif /* _ECHT_COMPONENT_H_ */
