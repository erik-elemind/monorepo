#ifndef _ECHT_SETTINGS_
#define _ECHT_SETTINGS_

class ECHTSettings {

public:
  // fft settings
  int fftSize;
  int fftLog2Size;
  int fftMirrorSize;
  int sampleSize;
  int zeroSize;
  int winSize;

  // filter settings
  int filtOrder;
  float centerFreq; // Hz
  float lowFreq; // Hz
  float highFreq; // Hz
  
  // input scale
  float inputScale; // multiple of input

  // sampling frequency of ECHT
  double sampFreq;

  void setFFTSize(int fft_size, int sample_size) {
    // save fft size
    fftSize = fft_size;
    // compute log 2 of fft_size
    fftLog2Size = 0;
    while (fft_size >>= 1) ++fftLog2Size;
    // save mirror
    fftMirrorSize = fftSize / 2;
    // save sample size
    sampleSize = sample_size;
    zeroSize = fftSize - sampleSize;
    // save win multiple
    winSize = fftSize;
  }

  void setFilterParams(int order, float centerFreq, float lowFreq, float highFreq){
    this->filtOrder  = order;
    this->centerFreq = centerFreq;
    this->lowFreq    = lowFreq;
    this->highFreq   = highFreq;
  }

  void setInputScale(float inputScale){
    this->inputScale = inputScale;
  }

  void setSampleFreq(double sampFreq){
    this->sampFreq = sampFreq;
  }
  
  ECHTSettings()
  {
    setFFTSize(1024,1024); // was 128
    setFilterParams(2, 5, 5-2.5, 5+2.5);    // FOR EEG: Filter from 7.5 - 12.5 Hz
    setInputScale(1);
    setSampleFreq(500);
  }
};

#endif // _ECHT_SETTINGS_