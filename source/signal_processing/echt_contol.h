#ifndef _ECHT_CONTROL_
#define _ECHT_CONTROL_

class ECHT_Control {
public:
  int fftSize;
  int fftLog2Size;
  int fftMirrorSize;
  int sampleSize;
  int zeroSize;
  int winSize;

  int filtOrder;
  float centerFreq; // Hz
  float lowFreq; // Hz
  float highFreq; // Hz
  float inputScale; // multiple of input

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

  ECHT_Control():
    filtOrder(2), centerFreq(5), lowFreq(-2.5), highFreq(2.5),   // FOR EEG: Filter from 7.5 - 12.5 Hz
    inputScale(-1)
  {
    setFFTSize(128,128);
  }
};

#endif // _ECHT_CONTROL_