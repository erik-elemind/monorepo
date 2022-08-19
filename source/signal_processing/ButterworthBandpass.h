/*********************************************************************************/
// VERSION 1 (Original) - Copied
// VERSION 2 - Added FREQZ computation, allowing the computation of the filter's
//             Fourier coefficients from its filter coefficients.

#ifndef _BUTTERWORTH_BANDPASS_H_
#define _BUTTERWORTH_BANDPASS_H_

//#include <Arduino.h>
#include <assert.h>
#include "iir.h"
#include "freqz.h"


template<typename T, int MAX_BB_ORDER_T>
class ButterworthBandpass{
  protected:
    size_t index_; // the index into the window array that will be updated next
    size_t numcof_;
    
    T dcof_[2*MAX_BB_ORDER_T+1];
    T ccof_[2*MAX_BB_ORDER_T+1];
    T x_[2*MAX_BB_ORDER_T+1];
    T y_[2*MAX_BB_ORDER_T+1];
    
    double dbwbp_rcof_[2*MAX_BB_ORDER_T];
    double dbwbp_tcof_[2*MAX_BB_ORDER_T];
    double dbwbp_dcof_[4*MAX_BB_ORDER_T];
    double cbwbp_tcof_[MAX_BB_ORDER_T+1];
    double cbwbp_ccof_[2*MAX_BB_ORDER_T+1];
  public:
    ButterworthBandpass(int order, double lowFreq, double highFreq, double sampleFreq){
      ButterworthBandpass();
      design(order,lowFreq,highFreq,sampleFreq);
    }
    ButterworthBandpass(): index_(0), numcof_(0){
    }
    ~ButterworthBandpass(){
    }
    void reset(){
      for(size_t i=0; i<numcof_; i++){
        x_[i] = 0;
        y_[i] = 0;
      }
    }
    void design(int order, double lowFreq, double highFreq, double sampleFreq, bool resetCache){
      double f1f = 2*lowFreq/sampleFreq;
      double f2f = 2*highFreq/sampleFreq;
      // compute coefficients and scaling factor
      dcof_bwbp( order, f1f, f2f, dbwbp_rcof_, dbwbp_tcof_, dbwbp_dcof_ );
      ccof_bwbp( order, cbwbp_tcof_, cbwbp_ccof_ );
      double sf = sf_bwbp( order, f1f, f2f );
      // allocate new arrays for filter and data
      numcof_ = 2*order+1;
      memset(dcof_,0,sizeof(dcof_)); // (2*order+1)
      memset(ccof_,0,sizeof(ccof_)); // (2*order+1)
      if(resetCache){
        memset(x_,0,sizeof(x_)); // (2*order+1)
        memset(y_,0,sizeof(y_)); // (2*order+1)
      }
      // copy coefficients to more efficient arrays
      for(size_t i=0; i<numcof_; i++){
        dcof_[i] = dbwbp_dcof_[i];
        ccof_[i] = cbwbp_ccof_[i]*sf;
      }
    }

    // y[n] = c0*x[n] + c1*x[n-1] + ... + cM*x[n-M] - ( d1*y[n-1] + d2*y[n-2] + ... + dN*y[n-N])
    // x[0] = oldest, x[n] = newest
    T step(T val){
      // setup for the next index;
      int curr_index = index_;
      index_++;
//      index_ = index_ % numcof_;
      if(index_ >= numcof_)
        index_ = 0;
      x_[curr_index] = val;
      y_[curr_index] = 0;
      T newy = 0;
      int cofi = numcof_-1;
      for(size_t i=index_; i<numcof_; i++, cofi--){
        newy += ccof_[cofi]*x_[i] - dcof_[cofi]*y_[i];
      }
      for(size_t i=0; i<index_; i++, cofi--){
        newy += ccof_[cofi]*x_[i] - dcof_[cofi]*y_[i];
      } 
      y_[curr_index] = newy;
      return newy;
    }
    
//    void printCoeffs(Stream &s){
//      // print dcof
//      s.print("dcof = ");
//      for(int i=0; i<numcof_; i++){
//        s.print(dcof_[i],4);
//        s.print(", ");
//      }
//      s.println();
//      // print ccof
//      s.print("ccof = ");
//      for(int i=0; i<numcof_; i++){
//        s.print(ccof_[i],4);
//        s.print(", ");
//      }
//      s.println();
//    }
    size_t getNumCof(){
      return numcof_;
    }
    T* getB(){
      return ccof_;
    }
    T* getA(){
      return dcof_;
    }
  };

template<typename T, int MAX_BB_ORDER_T, int MAX_BB_NFFT_T>
class ButterworthBandpassFreq : public ButterworthBandpass<T,MAX_BB_ORDER_T>{
  protected:
    T ffcr_[MAX_BB_NFFT_T/2+1]; // real component of fourier coeffcients
    T ffci_[MAX_BB_NFFT_T/2+1]; // imaginary component of fourier coefficients
  public:
    ButterworthBandpassFreq() : ButterworthBandpass<T,MAX_BB_ORDER_T>(){
    }
    ~ButterworthBandpassFreq(){
    }
    void designffc(size_t nfft){
      assert( (nfft/2+1) <= (sizeof(ffcr_)/sizeof(T)) );
      // use freqz to design filter fourier coeffs
      memset(ffcr_,0,sizeof(ffcr_)); // (nfft/2+1)
      memset(ffci_,0,sizeof(ffci_)); // (nfft/2+1)
      freqz(this->ccof_, this->dcof_, this->numcof_-1, nfft, ffcr_, ffci_);
    }
    void designffc(T* f, size_t flen, T Fs){
      // TODO: should this be less than or less than or equal to
      assert( flen < (sizeof(ffcr_)/sizeof(T)) );
      // use freqz to design filter fourier coeffs
      memset(ffcr_,0,sizeof(ffcr_)); // flen
      memset(ffci_,0,sizeof(ffci_)); // flen
      freqz(this->ccof_, this->dcof_, this->numcof_-1, f, flen, Fs, ffcr_, ffci_);
    }
    void designfiltfiltffc(size_t nfft){
      designffc(nfft);
      for (int  i = 0; i < this->numcof_; i++) {
        ffcr_[i] = sqrt(ffcr_[i]*ffcr_[i] + ffci_[i]*ffci_[i]);
        ffci_[i] = 0;
      }
    }
    void designfiltfiltffc(T* f, size_t flen, T Fs){
      designffc(f,flen,Fs);
      for (int  i = 0; i < this->numcof_; i++) {
        ffcr_[i] = sqrt(ffcr_[i]*ffcr_[i] + ffci_[i]*ffci_[i]);
        ffci_[i] = 0;
      }
    }
    T* getFFCr(){
      return ffcr_;
    }
    T* getFFCi(){
      return ffci_;
    }
};


#endif /* _BUTTERWORTH_BANDPASS_H_ */
