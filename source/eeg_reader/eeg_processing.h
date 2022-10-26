/*
 * alpha_switch.h
 *
 *  Created on: Aug 12, 2021
 *      Author: DavidWang
 */

#ifndef EEG_PROCESSING_H_
#define EEG_PROCESSING_H_


#define USE_LOG_MATH_FOR_ALPHA_ENV (0U)
#define USE_LOG_MATH_FOR_INST_RMS (0U)

#include <stdint.h>
#include <stddef.h>
#include "MovingAverage.h"
#include "eeg_datatypes.h"
#include "math.h"
#include "ButterworthBandpass.h"
#include "loglevels.h"
#include "fast_math.h"
#include "ble.h" // included to get ELECTRODE_NUM constant and ble_electrode_quality_update() function
#include "eeg_quality.h"
//#include "echt_impl4.h"
#include "ECHTImpl5.h"

#include "eeg_filters.h"
#include "eeg_quality.h"
#include "audio.h"
#include "data_log.h"
#include "HistoryVar.h"
#include "eeg_constants.h"

//static const char *TAG = "eeg_proc";  // Logging prefix for this module


#ifndef ECHT_ENABLE
#define ECHT_ENABLE (1U)
#endif


#ifndef ENABLE_EEG_FILTERS
#define ENABLE_EEG_FILTERS (1U)
#endif


#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))

#define MAX_WIN_SIZE 128 // num single-channel samples
#define MAX_FFT_SIZE 128 // num single-channel samples

#define MAX_BUTTERWORTH_BANDPASS_ORDER 2
#define MAX_BUTTERWORTH_BANDPASS_NFFT  MAX_FFT_SIZE

#endif


class EEGProcessing {
public:

  EEGProcessing(){
    init();
  }

  void init(){
    k = 0;
    k0 = 0;

#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))
    echt_chnum.init(init_echt_chnum, init_echt_chnum);
    pulse_trig.init(false,false);
#endif
    stim_on.init(true,true);
    stim_amp.init(1,1);

    first_run = true;
  }

  /***************************************************************************/
  // Filters
  /***************************************************************************/
private:
#if (defined(ENABLE_EEG_FILTERS) && (ENABLE_EEG_FILTERS > 0U))
  eeg_filters_context_t filters;
#endif

public:
#if (defined(ENABLE_EEG_FILTERS) && (ENABLE_EEG_FILTERS > 0U))
  void filters_init(){
    eeg_filters_init( &filters );
  }

  void line_filter_enable(bool enable){
    eeg_filters_enable_line_filter( &filters, enable );
  }

  void line_filter_config(int order, double cutOffFreq,  double sampFreq=250, bool resetCache = true){
    eeg_filters_config_line_filter( &filters, order, cutOffFreq );
  }

  void az_filter_enable(bool enable){
    eeg_filters_enable_az_filter( &filters, enable );
  }

  void az_filter_config(int order, double cutOffFreq, double sampFreq=250, bool resetCache = true){
    eeg_filters_config_az_filter( &filters, order, cutOffFreq );
  }
#else
  void filters_init(){}
  void line_filter_enable(bool enable){}
  void line_filter_config(int order, double cutOffFreq,  double sampFreq=250, bool resetCache = true){}
  void az_filter_enable(bool enable){}
  void az_filter_config(int order, double cutOffFreq, double sampFreq=250, bool resetCache = true){}
#endif


  /***************************************************************************/
  // ECHT
  /***************************************************************************/

private:

#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))
  // enable
  bool enable_echt;

  //  echt_impl4<MAX_WIN_SIZE, MAX_FFT_SIZE, MAX_BUTTERWORTH_BANDPASS_ORDER, MAX_BUTTERWORTH_BANDPASS_NFFT> echt;
  ECHTImpl5<MAX_WIN_SIZE, MAX_FFT_SIZE, MAX_BUTTERWORTH_BANDPASS_ORDER, MAX_BUTTERWORTH_BANDPASS_NFFT> echt0;
  ECHTImpl5<MAX_WIN_SIZE, MAX_FFT_SIZE, MAX_BUTTERWORTH_BANDPASS_ORDER, MAX_BUTTERWORTH_BANDPASS_NFFT> echt1;
  ECHTImpl5<MAX_WIN_SIZE, MAX_FFT_SIZE, MAX_BUTTERWORTH_BANDPASS_ORDER, MAX_BUTTERWORTH_BANDPASS_NFFT> echt2;

  // echt channel
  eeg_channel_t init_echt_chnum = 1;
  HistoryVar<eeg_channel_t> echt_chnum;  // ECHT processing channel (default start with Channel 1)

  float echt_min_phase_rad;
  float echt_max_phase_rad;

  HistoryBool pulse_trig;

  bool pink_is_playing;
#endif // end defined(ECHT_ENABLE)

#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))

size_t triggered_sample_count = 0;
size_t triggered_sample_count_reset = 6;

#endif // end defined(ECHT_ENABLE)

public:

#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))
  void echt_config(int fft_size, int filter_order, float center_freq, float low_freq, float high_freq, float input_scale, double sample_freq) {
    echt0.setCntrl(fft_size, filter_order, center_freq, low_freq, high_freq, input_scale, sample_freq);
    echt1.setCntrl(fft_size, filter_order, center_freq, low_freq, high_freq, input_scale, sample_freq);
    echt2.setCntrl(fft_size, filter_order, center_freq, low_freq, high_freq, input_scale, sample_freq);
  }

  void echt_set_channel(eeg_channel_t channel_number){
    echt_chnum.set(channel_number);
  }

  void echt_set_init_channel(uint8_t init_echt_channel){
    init_echt_chnum = init_echt_channel;
    echt_chnum.set(init_echt_channel);
  }

  void echt_enable(bool enable){
    enable_echt = enable;
  }

  void echt_set_min_max_phase(float min_rad, float max_rad){
    echt_min_phase_rad = min_rad;
    echt_max_phase_rad = max_rad;
  }
#else
  void echt_config(int fft_size, int filter_order, float center_freq, float low_freq, float high_freq, float input_scale, double sample_freq) {}
  void echt_set_channel(eeg_channel_t channel_number){}
  void echt_set_init_channel(uint8_t init_echt_channel){}
  void echt_enable(bool enable){}
  void echt_set_min_max_phase(float min_rad, float max_rad){}
#endif

  /***************************************************************************/
  // CHANNEL SWITCHING AND ALPHA THRESHOLDING
  /***************************************************************************/

private:
  // enable
  bool enable_alpha_thresh = false;
  bool enable_channel_switch = false;

  // Onset/Offset Ramp decay rates
  float dur1_sec = 5;    // Offset decay duration (seconds)
  float dur2_sec = 10;   // Onset growth duration (seconds)
  float timedLockOut_sec = 1; // Timed lock-out duration before resuming stimulus (seconds)
  float minRMSPower_uV = 2; // minimum EEG RMS signal power for drop-out detection
  float minRMSPower_dB = 0; // 20log10 of minRMSPower
  float alphaThr_dB = 6; // Alpha envelope power threshold (dB)
  float alphaThr_uV = 0;

  float dur1_step = 0;
  float dur2_step = 0;

  const size_t nFFT = 128;     // ECHT analysis window length (samples)
  const size_t fs = 250;       // ECHT sampling rate (samples/sec)

  float kLO;    // lock-out duration in samples
  uint64_t k0;  // initialize timed lock-out event to sample 0 (MATLAB referenced)
  uint64_t k;   // current sample time;

  float prev_amp = 1;

  HistoryBool stim_on;
  HistoryVar<float> stim_amp;

  bool first_run = true;

  MovingAverage<float, float, float, 128> rms[MAX_NUM_EEG_CHANNELS];
  MovingAverage<float, float, float, 128> alphaEnv[MAX_NUM_EEG_CHANNELS];
  float instRMS[MAX_NUM_EEG_CHANNELS];
  int32_t eeg[MAX_NUM_EEG_CHANNELS];
  float alpha[MAX_NUM_EEG_CHANNELS];


public:
  void alpha_switch_enable(bool enable){
    enable_alpha_thresh = enable;
  }

  void channel_switch_enable(bool enable){
    enable_channel_switch = enable;
  }


  void set_params(float dur1_sec, float dur2_sec, float timedLockOut_sec, float minRMSPower_uV, float alphaThr_dB){
    this->dur1_sec = dur1_sec;
    this->dur2_sec = dur2_sec;
    this->timedLockOut_sec = timedLockOut_sec;
    this->minRMSPower_uV = minRMSPower_uV;
    this->minRMSPower_dB = 20*log10f_fast(minRMSPower_uV);
    this->alphaThr_dB = alphaThr_dB;
    this->alphaThr_uV = pow10f_fast(alphaThr_dB/20.0f);
    this->kLO = round(timedLockOut_sec*fs); // lock-out duration in samples
    this->prev_amp = 1;
    this->dur1_step = 1.0f/(fs*dur1_sec);
    this->dur2_step = 1.0f/(fs*dur2_sec);
  }

  void compute_instRMS( int32_t* eeg ){
    // save off eeg in case it is needed
    memcpy(this->eeg, eeg, MAX_NUM_EEG_CHANNELS);
    // compute instRMS
    for(uint8_t i=0; i<MAX_NUM_EEG_CHANNELS; i++){
#if (defined(USE_LOG_MATH_FOR_INST_RMS) && (USE_LOG_MATH_FOR_INST_RMS > 0U))
      float eeg_sq_dB = 10*log10f_fast(rms[i].average(eeg[i]*EEG_SCALAR_SQUARED*eeg[i]));
      if(isfinite(eeg_sq_dB)){
        instRMS[i] = eeg_sq_dB;
      }else{
        instRMS[i] = -INFINITY;
      }
#else
      float eeg_scaled = eeg[i]*EEG_SCALAR_UV;
      if(isfinite(eeg_scaled)){
        instRMS[i] = rms[i].average(abs(eeg_scaled));
      }else{
        instRMS[i] = -INFINITY;
      }
#endif
    }
//    LOGV("select_channel", "instRMS: %f %f %f", instRMS[0], instRMS[1], instRMS[2]);
  }


  void compute_alphaEnv(float* alpha){
    // save off eeg in case it is needed
    memcpy(this->alpha, alpha, MAX_NUM_EEG_CHANNELS);
    // Update all the alpha envelope moving averages
    for(uint8_t i=0; i<MAX_NUM_EEG_CHANNELS; i++){
      // TODO: This alpha filtering
#if (defined(USE_LOG_MATH_FOR_ALPHA_THRESHOLD) && (USE_LOG_MATH_FOR_ALPHA_THRESHOLD > 0U))
      float alpha_sq_dB = 10*log10f_fast(alpha[i]*EEG_SCALAR_SQUARED*alpha[i]);
      if(isfinite(alpha_sq_dB)){
        alphaEnv[i].average(alpha_sq_dB);
      }
#else
      float alpha_scaled = alpha[i]*EEG_SCALAR_UV;
      if(isfinite(alpha_scaled)){
        alphaEnv[i].average(abs(alpha_scaled));
      }
#endif
    }
  }

#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))

  // The call to select_channel() assumes compute_alphaEnv() and compute_instRMS has been called in advance on any new eeg and alpha data.
  uint8_t select_channel( bool &stimOn, float &amp ){

    uint8_t echt_chnum_local = echt_chnum.get();

//    LOGV("select_channel", "amp: %f %f %f", alpha[0], alpha[1], alpha[2]);
//    LOGV("select_channel", "average: %f %f %f", alphaEnv[0].average(), alphaEnv[1].average(), alphaEnv[2].average());

    bool isDropOut[MAX_NUM_EEG_CHANNELS];
    bool isDropOutAll = true;

    // Calculate RMS energy for samples [k-127:k] MATLAB referencing
    for(uint8_t i=0; i<MAX_NUM_EEG_CHANNELS; i++){
#if (defined(USE_LOG_MATH_FOR_INST_RMS) && (USE_LOG_MATH_FOR_INST_RMS > 0U))
      float eeg_sq_dB = 10*log10f_fast(rms[i].average(eeg[i]*EEG_SCALAR_SQUARED*eeg[i]));
      if( instRMS[i] != -INFINITY ){
        isDropOut[i] = instRMS[i]<minRMSPower_dB;
      }else{
        isDropOut[i] = true;
      }
      isDropOutAll = isDropOutAll && isDropOut[i];
#else
      if( instRMS[i] != -INFINITY ){
        isDropOut[i] = instRMS[i]<minRMSPower_uV;
      }else{
        isDropOut[i] = true;
      }
      isDropOutAll = isDropOutAll && isDropOut[i];
#endif
    }

//    LOGV("select_channel", "minRMSPower, %f uV,  %f dB", minRMSPower_uV ,minRMSPower_dB);


    // Is outside of time lock out interval?
    if( k<=(k0+kLO) ){
//      LOGV("alpha_switch","in lockout");
      stimOn = false;
      amp = fmax(prev_amp-dur1_step,0);
    } else {
//      LOGV("alpha_switch","outside lockout");

      if(isDropOutAll){
//        LOGV("alpha_switch","is drop out all");
        // No signal on all channels?
        stimOn = false; // Set Stimulus to OFF
        amp = fmax(prev_amp-dur1_step,0); // Decay stimulus amplitude by r1 (min=0)
        k0 = k;   // start time lock out clock
      }else{
        // If signal on current channel drops out
#if (defined(USE_LOG_MATH_FOR_INST_RMS) && (USE_LOG_MATH_FOR_INST_RMS > 0U))
        bool less_than_rms_power = instRMS[echt_chnum_local] < minRMSPower_dB;
#else
        bool less_than_rms_power = instRMS[echt_chnum_local] < minRMSPower_uV;
#endif
        if( less_than_rms_power ){
          // First, find alternative channels with RMS energy > RMS threshold

          // Then, pick channel with the strongest signal (will figure out
          // how to limit this to exclude movement/EMG artifact later)
          float instRMSMax = instRMS[echt_chnum_local];
          for(uint8_t i=0; i<MAX_NUM_EEG_CHANNELS; i++){
            if( instRMS[i]>instRMSMax ){
              instRMSMax = instRMS[i];
              echt_chnum_local = i;
            }
          }
        }

        // replaces alpha thresholding here.
        stimOn = true;
        amp = fmin(prev_amp+dur2_step,1);

      }

    }

    // increment k
    k++;
    prev_amp = amp;
    echt_chnum.set( echt_chnum_local );
    return echt_chnum_local;
  }


  void alpha_threshold(bool &stimOn, float &amp){
    // Next check the strength of the alpha signal
#if (defined(USE_LOG_MATH_FOR_ALPHA_THRESHOLD) && (USE_LOG_MATH_FOR_ALPHA_THRESHOLD > 0U))
    bool greater_than_alpha_threshold = alphaEnv[echt_chnum.get()].average() >= alphaThr_dB;
#else
    bool greater_than_alpha_threshold = alphaEnv[echt_chnum.get()].average() >= alphaThr_uV;
#endif
    if( greater_than_alpha_threshold ){
      stimOn = true;
      amp = fmin(prev_amp+dur2_step,1);
    }else{
      stimOn = false;
      amp = prev_amp;
    }
  }

#endif // (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))

  /***************************************************************************/
  // EEG Quality Test
  /***************************************************************************/

private:
  EEGQualityTest eegtest;

public:

  void start_blink_test(){
    eegtest.start_blink_test();
  }

  void stop_blink_test(){
    eegtest.stop_blink_test();
  }

  void start_electrode_quality_test(){
    eegtest.start_electrode_quality_test();
  }

  void stop_electrode_quality_test(){
    eegtest.stop_electrode_quality_test();
  }

  /***************************************************************************/
  // PROCESS ALL
  /***************************************************************************/

public:
  void process(ads129x_frontal_sample* f_sample){
#if (defined(ENABLE_EEG_FILTERS) && (ENABLE_EEG_FILTERS > 0U))
    // filter the EEG data
    eeg_filters_filter( &filters, f_sample);
#endif //ENABLE_EEG_FILTERS

    data_log_eeg( f_sample );

    // log initial values
    if(first_run){
      first_run = false;
      // save off initial delta parameters
#if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))
      data_log_echt_channel(f_sample->sample_number, echt_chnum.get());
#endif
      // log stimulus on
      data_log_stimulus_switch(f_sample->sample_number, stim_on.get());
      // log stimulus amplitude
      data_log_stimulus_amplitude(f_sample->sample_number,stim_amp.get());
    }

    compute_instRMS(f_sample->eeg_channels);

    eegtest.run( f_sample, instRMS );

    // compute ECHT
  #if (defined(ECHT_ENABLE) && (ECHT_ENABLE > 0U))
    // always add data to echt, so it is primed when we turn it on.
    // add the sample to the ecHT algorithm
    echt0.addData( f_sample->eeg_channels[0] );
    echt1.addData( f_sample->eeg_channels[1] );
    echt2.addData( f_sample->eeg_channels[2] );

    if (enable_echt) {
      bool channel_switch_stim_on = true;
      float channel_switch_amp = 1;
      bool alpha_thresh_stim_on = true;
      float alpha_thresh_amp = 1;

      // save history
      echt_chnum.backup();
      stim_on.backup();
      stim_amp.backup();

      // compute channel switch
      if (enable_channel_switch){
        select_channel( channel_switch_stim_on, channel_switch_amp );
      }

      // decide how to run, then run echt, AFTER the new channel is selected
      bool run_echt_all_channels = enable_alpha_thresh;
      // variables to store echt values
      float eeg_now, inst_amp, inst_phs;
      float eeg_now_arr[MAX_NUM_EEG_CHANNELS];
      float inst_amp_arr[MAX_NUM_EEG_CHANNELS];
      float inst_phs_arr[MAX_NUM_EEG_CHANNELS];

      if(run_echt_all_channels){
        echt0.computeInstAmpPhase(eeg_now_arr[0], inst_amp_arr[0], inst_phs_arr[0]);
        echt1.computeInstAmpPhase(eeg_now_arr[1], inst_amp_arr[1], inst_phs_arr[1]);
        echt2.computeInstAmpPhase(eeg_now_arr[2], inst_amp_arr[2], inst_phs_arr[2]);

        eeg_now = eeg_now_arr[echt_chnum.get()];
        inst_amp = inst_amp_arr[echt_chnum.get()];
        inst_phs = inst_phs_arr[echt_chnum.get()];
      }else{
        switch(echt_chnum.get()){
        case 0:
          echt0.computeInstAmpPhase(eeg_now, inst_amp, inst_phs);
          break;
        case 1:
          echt1.computeInstAmpPhase(eeg_now, inst_amp, inst_phs);
          break;
        case 2:
          echt2.computeInstAmpPhase(eeg_now, inst_amp, inst_phs);
          break;
        }
      }

      // run ecHT algorithm
      if (enable_alpha_thresh){
        // compute channel switching
        compute_alphaEnv(inst_amp_arr);
        alpha_threshold(alpha_thresh_stim_on, alpha_thresh_amp);
      }

//      LOGV("eeg_proc", "channel_switch_stim_on: %d", channel_switch_stim_on);
//      LOGV("eeg_proc", "alpha_thresh_stim_on: %d", alpha_thresh_stim_on);

      stim_on.set( channel_switch_stim_on && alpha_thresh_stim_on );
      stim_amp.set( channel_switch_amp * alpha_thresh_amp );

      // save off new data
      audio_pink_computed_volume(stim_amp.get());

      // log echt channel
//      LOGV("alpha_switch","echt_channel_number: %u", echt_channel_number);
      if(echt_chnum.changed()){
        data_log_echt_channel(f_sample->sample_number, echt_chnum.get());
      }

      // log stimulus on
      if(stim_on.changed()){
        data_log_stimulus_switch(f_sample->sample_number, stim_on.get());
      }

      // log stimulus amplitude
      if (stim_amp.changed()){
        data_log_stimulus_amplitude(f_sample->sample_number,stim_amp.get());
      }

    //  LOGV(TAG,"amp: %.10f, phs: %.10f", instAmp, instPhs);

      // TODO: THe following call causes a virtual_com.c crash, where the USB ISR is never called.
      // log amplitude and phase
      data_log_inst_amp_phs(f_sample->sample_number, inst_amp, inst_phs);

      // compute the pulse
      // TODO: Move these phase parameters to one global struct
      float phase_correction = -0.122;
      float targeted_phase = echt_min_phase_rad;
      float pulse_start_phase = (targeted_phase + phase_correction);
      float pulse_stop_phase = (targeted_phase + phase_correction) + 0.6;
      float pulse_phase_offset = 0;
      pulse_trig.backup();
      pulse_trig.set(wrapTo2Pi(pulse_stop_phase - pulse_start_phase) > wrapTo2Pi(inst_phs - (pulse_start_phase + pulse_phase_offset)));

      triggered_sample_count++;

        //  LOGV(TAG, "%d %d", pulse_trig_prev, pulse_trig_curr);
        // TODO: Replace the gating of audio with a mixer-style volume control
        if ( pulse_trig.F2T() ) {
          if (!pink_is_playing){
            pink_is_playing = true;

            triggered_sample_count = 0;
            // if the phase of brainwave entered the region we want to "pulse", UNPAUSE the audio
            audio_pink_mute(false);
            // log the pulse start
            data_log_pulse(f_sample->sample_number, true);
          }

        }else if ( /*(!pulse_trig_curr && pulse_trig_prev) ||*/ triggered_sample_count > triggered_sample_count_reset ) {
          if(pink_is_playing){
            pink_is_playing = false;

            // if the phase of brainwave exists the region we want to "pulse", PAUSE the audio
            audio_pink_mute(true);
            // log the pulse end
            data_log_pulse(f_sample->sample_number, false);
          }
        }

    } //  if(enable_echt)

  #endif
  }




};


#endif /* EEG_PROCESSING_H_ */
