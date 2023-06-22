#ifndef _EEG_FILTER_H_
#define _EEG_FILTER_H_

#include "string.h" // for memset
//#include "ButterworthBandpass.h"
//#include "ButterworthBandstop.h"
//#include "ButterworthHighpass.h"
//#include "ButterworthLowpass.h"
#include "ButterworthLowpassBiQuad.h"
#include "loglevels.h"

template<typename T, int MAX_EEG_FILT_ORDER>
class eeg_filter {
private:

    ButterworthLowpassBiQuad<T,MAX_EEG_FILT_ORDER> lpfilter45;
    ButterworthLowpassBiQuad<T,MAX_EEG_FILT_ORDER> hpfilter;

public:

    T filter(T eeg_volts, bool enable_line, bool enable_az){
        if(enable_line){
          eeg_volts = lpfilter45.step(eeg_volts);
        }
        if(enable_az){
          eeg_volts = eeg_volts - hpfilter.step(eeg_volts);
        }
        return eeg_volts;
    }

    void designLineFilters(int order, double cutoffFreq, double sampleFreq, bool resetCache){
        lpfilter45.design(order, cutoffFreq, sampleFreq, resetCache);
    }

    void designAZFilters(int order, double cutoffFreq, double sampleFreq, bool resetCache){
        hpfilter.design(order, cutoffFreq, sampleFreq, resetCache);
    }

};


#endif //_EEG_FILTER_H_
