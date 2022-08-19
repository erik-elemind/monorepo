/*
 * eeg_quality.h
 *
 *  Created on: May 29, 2021
 *      Author: DavidWang
 */

#ifndef EEG_READER_EEG_QUALITY_H_
#define EEG_READER_EEG_QUALITY_H_

#include "FreeRTOS.h" // for configASSERT() function

#include "ads129x.h"
#include "eeg_datatypes.h"
#include "HistoryVar.h"
#include "audio.h"
#include "ble_uart_send.h"
#include "eeg_reader.h"
#include "eeg_constants.h"
#include "interpreter.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef enum{
  ELECTRODE_QUALITY_NO_SIGNAL = 0,
  ELECTRODE_QUALITY_WEAK_SIGNAL = 150,
  ELECTRODE_QUALITY_GOOD_SIGNAL = 250,
  ELECTRODE_QUALITY_ARTIFACT = 0,
} electrode_quality_t;


#define ELECTRODE_NUM_FOR_QUALITY ELECTRODE_NUM

class EEGQualityTest{

  /***************************************************************************/
  // ELECTRODE QUALITY
  /***************************************************************************/

private:
  uint8_t electrode_index[3] = {EEG_FP1, EEG_FPZ, EEG_FP2};
  size_t eeg_quality_drop_counter = 0;

  void compute_eeg_quality( float* instRMS ){
    // only compute eeg quality every 250 samples.
    if(eeg_quality_drop_counter % 250 != 0){
      return;
    }

    uint8_t electrode_quality[ELECTRODE_NUM_FOR_QUALITY] = { 0 };

    for(uint8_t i=0; i<MAX_NUM_EEG_CHANNELS; i++){

#if (defined(USE_LOG_MATH_FOR_INST_RMS) && (USE_LOG_MATH_FOR_INST_RMS > 0U))
      uint8_t ei = electrode_index[i];
      if (instRMS[i] < 1){
        electrode_quality[ei] = ELECTRODE_QUALITY_NO_SIGNAL;
      } else if ( instRMS[i] < 5) {
        electrode_quality[ei] = ELECTRODE_QUALITY_WEAK_SIGNAL;
      } else if ( instRMS[i] <= 500) {
        electrode_quality[ei] = ELECTRODE_QUALITY_GOOD_SIGNAL;
      } else {
        electrode_quality[ei] = ELECTRODE_QUALITY_ARTIFACT;
      }
#else
      // float eeg_sq_dB = 10*log10f_fast(rms[i].average(eeg[i]*scalar_squared*eeg[i]));
      // Conversion from DB TO LINEAR: LIN_VALUE = sqrt(10^(DB_VALUE/10))
      // float eeg_sq_dB = 10*log10f_fast(rms[i].average(eeg[i]*EEG_SCALAR_SQUARED*eeg[i]));
      // float eeg_scaled = eeg[i]*EEG_SCALAR;

      uint8_t ei = electrode_index[i];
      if (instRMS[i] < 1.0f){ /*supposed to be 1.1220184543f*/
        electrode_quality[ei] = ELECTRODE_QUALITY_NO_SIGNAL;
      } else if ( instRMS[i] < 5.0f) { /*supposed to be 1.77827941004f*/
        electrode_quality[ei] = ELECTRODE_QUALITY_WEAK_SIGNAL;
      } else if ( instRMS[i] <= 500.0f) {
        electrode_quality[ei] = ELECTRODE_QUALITY_GOOD_SIGNAL;
      } else {
        electrode_quality[ei] = ELECTRODE_QUALITY_ARTIFACT;
      }
#endif

    }

    //2-4 when low
//    LOGV("eeg_quality", "instRMS: %f %f %f", instRMS[0], instRMS[1], instRMS[2]);

    // report electrode quality
    ble_electrode_quality_update(electrode_quality);
//    LOGV("eeg_quality", "quality: %u %u %u", electrode_quality[0], electrode_quality[1], electrode_quality[2]);

  }

  /***************************************************************************/
  // BLINK TEST
  /***************************************************************************/

private:
  float blink_vmin[MAX_NUM_EEG_CHANNELS] = {0};
  float blink_vmax[MAX_NUM_EEG_CHANNELS] = {0};

  unsigned long blink_kmin[MAX_NUM_EEG_CHANNELS] = {0};
  unsigned long blink_kmax[MAX_NUM_EEG_CHANNELS] = {0};

  float blink_thresh_uV_hi = 120; // uV
  float blink_thresh_uV_lo = -80; // uV

  HistoryBool blink_change;

  void blink_init(){
    memset(blink_vmin,0,sizeof(blink_vmin));
    memset(blink_vmax,0,sizeof(blink_vmax));
    memset(blink_kmin,0,sizeof(blink_kmin));
    memset(blink_kmax,0,sizeof(blink_kmax));
    blink_change.init(false, false);
  }

  bool compute_blink(  unsigned long sample_number, int32_t* eeg ){
    bool blink_is_blink[MAX_NUM_EEG_CHANNELS] = {false};
    uint8_t blink_count = 0;

    // uses eeg data
    for(uint8_t i=0; i<MAX_NUM_EEG_CHANNELS; i++){
      float eeg_scaled = eeg[i]*EEG_SCALAR_UV;

      if(eeg_scaled > blink_vmax[i] && eeg_scaled > blink_thresh_uV_hi){
        blink_vmax[i] = eeg_scaled;
        blink_kmax[i] = sample_number;
      }

      if(eeg_scaled < blink_vmin[i] && eeg_scaled < blink_thresh_uV_lo){
        blink_vmin[i] = eeg_scaled;
        blink_kmin[i] = sample_number;
      }

      blink_is_blink[i] =
          (blink_vmax[i]>blink_thresh_uV_hi) &&
          (blink_vmin[i]<blink_thresh_uV_lo) &&
          (blink_kmax[i]<blink_kmin[i]);

      if(blink_is_blink[i]){
        blink_count++;
      }
    }
//    LOGV("eeg_quality","%f %f %f | %f", eeg[0]*EEG_SCALAR, eeg[1]*EEG_SCALAR, eeg[2]*EEG_SCALAR , blink_thresh_uV);

    // return a blink if detected on 2 out of 3 channels.
    blink_change.backup();
    blink_change.set(blink_count>=2);
#if 1
    return blink_change.F2T();
#else
    return true;
#endif
  }

  bool init_blink_test = false;
  bool run_blink_test = false;
  bool run_quality_check = false;
  HistoryBool blink_detected;

public:

  void start_blink_test() {
    run_blink_test = true;
    init_blink_test = true;
  }

  void stop_blink_test() {
    run_blink_test = false;
  }

  void start_electrode_quality_test(){
    run_quality_check = true;
  }

  void stop_electrode_quality_test(){
    run_quality_check = false;
  }

  void run( ads129x_frontal_sample* f_sample, float* instRMS ){
    if(init_blink_test){
      init_blink_test = false;
      blink_detected.init(false, false);
      blink_init();
    }
    if(run_blink_test){
      bool is_blink = compute_blink(f_sample->sample_number , f_sample->eeg_channels);
      blink_detected.backup();
      blink_detected.set(is_blink);
      if(blink_detected.F2T()){
        interpreter_event_blink_detected();
      }
    }

    if(run_quality_check){
      compute_eeg_quality( instRMS );
    }
  }

};


#ifdef __cplusplus
}
#endif

#endif /* EEG_READER_EEG_QUALITY_H_ */
