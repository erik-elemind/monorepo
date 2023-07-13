#ifndef _EEG_THRESHOLDING_H_
#define _EEG_THRESHOLDING_H_

#include "Interface/Stream/StreamCommand.h"
#include "Interface/Stream/StreamCodes.h"
#include "Interface/InterfaceAdapter.h"
#include "CustomCommandCodes.h"

#include "SignalProcessing/ButterworthLowpass.h"
#include "SignalProcessing/ButterworthLowpassBiQuad.h"

class EEGThresholdingSettings
{
public:
    bool eeg_thresholding_enable;
    float eeg_thresholding_sigmoid_thresh;
    float eeg_thresholding_sigmoid_slope;
    float eeg_thresholding_am_low_mag_limit;

    bool eegth_enable_man_min;
    float eegth_man_min;
    bool eegth_enable_man_max;
    float eegth_man_max;
    float eegth_avg_dur_sec;

    // sampling frequency of ECHT
    double sampFreq;

    EEGThresholdingSettings() : eeg_thresholding_enable(false), eeg_thresholding_sigmoid_thresh(0.5), eeg_thresholding_sigmoid_slope(15), eeg_thresholding_am_low_mag_limit(0.0316),
                                eegth_enable_man_min(true), eegth_man_min(-100), eegth_enable_man_max(true), eegth_man_max(100), eegth_avg_dur_sec(1),
                                sampFreq(500)
    {
    }
};

class EEGThresholding
{
public:
    // CONTROL DATA
    EEGThresholdingSettings pendSet;
    EEGThresholdingSettings currSet;

protected:
    StreamCommandArray *strCmdArr_;
    InterfaceAdapter *userITF_;

#if 0
    ButterworthLowpass<double, 5> eegth_env_avg_filter1;
    ButterworthLowpass<double, 5> eegth_env_avg_filter2;
#else
    ButterworthLowpassBiQuad<double,5> eegth_env_avg_filter_biquad;
#endif

    float volume_am;
    float eegth_env_avg;
    float eegth_env_min;
    float eegth_env_max;

    // STREAM COMMANDS
    void initStreamCommands()
    {
        strCmdArr_->set(SC_VOL_AM, (char *)"volam", [this](Writer &w) {
            w.writeFLOAT(volume_am);
        });
        strCmdArr_->set(SC_EEGTH_AVG, (char *)"eegthavg", [this](Writer &w) {
            w.writeFLOAT(eegth_env_avg);
        });
        strCmdArr_->set(SC_EEGTH_MIN, (char *)"eegthmin", [this](Writer &w) {
            w.writeFLOAT(eegth_env_min);
        });
        strCmdArr_->set(SC_EEGTH_MAX, (char *)"eegthmax", [this](Writer &w) {
            w.writeFLOAT(eegth_env_max);
        });
    }

    // MESSAGE PROCESSING
    void initCommands()
    {
        userITF_->addHandler(CC_EEGTH_EN, [this](Reader *r) {
            auto enable = r->readBOOL();
            if (enable.isError())
            {
                return;
            }
            pendSet.eeg_thresholding_enable = enable.getValue();
        });

        userITF_->addHandler(CC_EEGTH_MAG, [this](Reader *r) {
            auto eegth_sigmoid_thresh = r->readFLOAT();
            auto eegth_sigmoid_slope = r->readFLOAT();
            auto eegth_am_low_mag_limit = r->readFLOAT();
            if (eegth_sigmoid_thresh.isError() || eegth_sigmoid_slope.isError() || eegth_am_low_mag_limit.isError())
            {
                return;
            }
            pendSet.eeg_thresholding_sigmoid_thresh = eegth_sigmoid_thresh.getValue();
            pendSet.eeg_thresholding_sigmoid_slope = eegth_sigmoid_slope.getValue();
            pendSet.eeg_thresholding_am_low_mag_limit = eegth_am_low_mag_limit.getValue();
        });
        userITF_->addHandler(CC_EEGTH_DB, [this](Reader *r) {
            auto eegth_sigmoid_thresh = r->readFLOAT();
            auto eegth_sigmoid_slope = r->readFLOAT();
            auto eegth_am_low_db_limit = r->readFLOAT();
            if (eegth_sigmoid_thresh.isError() || eegth_sigmoid_slope.isError() || eegth_am_low_db_limit.isError())
            {
                return;
            }
            pendSet.eeg_thresholding_sigmoid_thresh = eegth_sigmoid_thresh.getValue();
            pendSet.eeg_thresholding_sigmoid_slope = eegth_sigmoid_slope.getValue();
            pendSet.eeg_thresholding_am_low_mag_limit = db2mag(eegth_am_low_db_limit.getValue());
        });
        userITF_->addHandler(CC_EEGTH_MANUAL_MIN, [this](Reader *r) {
            auto eegth_man_min = r->readFLOAT();
            if (eegth_man_min.isError())
            {
                return;
            }
            pendSet.eegth_enable_man_min = true;
            pendSet.eegth_man_min = eegth_man_min.getValue();
        });
        userITF_->addHandler(CC_EEGTH_MANUAL_MAX, [this](Reader *r) {
            auto eegth_man_max = r->readFLOAT();
            if (eegth_man_max.isError())
            {
                return;
            }
            pendSet.eegth_enable_man_max = true;
            pendSet.eegth_man_max = eegth_man_max.getValue();
        });
        userITF_->addHandler(CC_EEGTH_AUTO_MIN, [this](Reader *r) {
            pendSet.eegth_enable_man_min = false;
        });
        userITF_->addHandler(CC_EEGTH_AUTO_MAX, [this](Reader *r) {
            pendSet.eegth_enable_man_max = false;
        });
        userITF_->addHandler(CC_EEGTH_AVG, [this](Reader *r) {
            auto eegth_avg_dur_sec = r->readFLOAT();
            if (eegth_avg_dur_sec.isError())
            {
                return;
            }
            pendSet.eegth_avg_dur_sec = eegth_avg_dur_sec.getValue();
        });
    }

    inline void resetRunningData(){
        volume_am = 0;
        eegth_env_avg = 0;
        eegth_env_min = 0;
        eegth_env_max = 0;
    }

public:
    EEGThresholding(StreamCommandArray *strCmdArr, InterfaceAdapter *userITF) : strCmdArr_(strCmdArr), userITF_(userITF)
    {
        resetRunningData();
    }

    ~EEGThresholding()
    {
    }

    void init(){
        initStreamCommands();
        initCommands();
        applyCntrl(true);
    }

    void applyCntrl(boolean force)
    {
        // design averaging
        if(force || 
           (pendSet.eegth_avg_dur_sec != currSet.eegth_avg_dur_sec)){
            double cutFreq = 1.0 / pendSet.eegth_avg_dur_sec;
#if 0
            eegth_env_avg_filter1.design(2, cutFreq, pendSet.sampFreq, false);
            eegth_env_avg_filter2.design(1, cutFreq, pendSet.sampFreq, false);
#else
            eegth_env_avg_filter_biquad.design(4, cutFreq, pendSet.sampFreq, false);
#endif
        }

        currSet = pendSet;
    }

  /**************************************/
  /* PARAMETER MANIPULATION FUNCTIONS   */

    void setSampleFreq(double sampFreq){
        pendSet.sampFreq = sampFreq;
    }

    bool isEnabled()
    {
        return currSet.eeg_thresholding_enable;
    }

    float computeAM(double instAmp)
    {
        if (!currSet.eeg_thresholding_enable)
        {
            return (this->volume_am = 1);
        }
        // compute the mix max
        // TODO: compute EEG min max
        float eegth_env_min = 0;
        if (currSet.eegth_enable_man_min) {
            eegth_env_min = currSet.eegth_man_min;
        } else {
            // TODO: implement auto min computation
        }
        float eegth_env_max = 0;
        if (currSet.eegth_enable_man_max) {
            eegth_env_max = currSet.eegth_man_max;
        } else {
            // TODO: implement auto max computation
        }
        // Filter the instantaneous amplitude
        float eegth_env_avg = instAmp;
#if 0
        eegth_env_avg = eegth_env_avg_filter1.step(eegth_env_avg);
        eegth_env_avg = eegth_env_avg_filter2.step(eegth_env_avg);
        if (isnan(eegth_env_avg)) {
            eegth_env_avg_filter1.reset();
            eegth_env_avg_filter2.reset();
        }
#else
        eegth_env_avg = eegth_env_avg_filter_biquad.step(eegth_env_avg);
        if (isnan(eegth_env_avg)) {
            eegth_env_avg_filter_biquad.reset();
        }
#endif
        // compute the thresholding
        float thresh = currSet.eeg_thresholding_sigmoid_thresh;
        float slope = currSet.eeg_thresholding_sigmoid_slope;
        float am_low_mag_limit = currSet.eeg_thresholding_am_low_mag_limit;
        float am = threshold_mag(eegth_env_avg, eegth_env_min, eegth_env_max, thresh, slope, am_low_mag_limit);
        // record the signal data
        this->volume_am = am;
        this->eegth_env_avg = eegth_env_avg;
        this->eegth_env_min = eegth_env_min;
        this->eegth_env_max = eegth_env_max;
        return am;
    }

    // Thresholding function
    //
    // INPUT VARIABLES
    //       x : raw EEG signal
    //       a : threshold (50%) parameter (default: 0.5)
    //       b : slope parameter (default: 15)
    //       g : amplitude modulation lower limit (dB)
    // OUTPUT VARIABLE
    //      am : amplitude modulation signal
    //
    // Created: 2020-09-15 scott.bressler@elemindtech.com
    float threshold_mag(float eeg, float eeg_min, float eeg_max, float thresh, float slope, float am_low_limit_mag)
    {
        float x = eeg;
        float a = thresh;
        float b = slope;
        float g = am_low_limit_mag;
        // Apply min-max normalization to input signal, x
        x = (eeg - eeg_min) / (eeg_max - eeg_min); // min-max normalization

        // Generate gain attenuation vector (sigmoid function)
        // am = g+(1-g)*(1./(1+exp(-b.*(x-a))));
        float am = g + (1 - g) * (1 / (1 + exp(-b * (x - a))));
        return am;
    }

    float db2mag(float ydb)
    {
        return pow(10, (ydb / 20));
    }
};

#endif //_EEG_THRESHOLDING_H_