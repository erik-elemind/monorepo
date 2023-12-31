#ifndef ML_UTILS_H
#define ML_UTILS_H

//TODO fix vector init to static at init

#include "ml.h"
#include "ButterworthHighpass.h"
#include "ButterworthBandpass.h"

#include <complex>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include "memman_rtos.h"
#include <iostream>

/***************************************************************************
Copyright (c) 2009, Motorola, Inc

All Rights Reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright 
notice, this list of conditions and the following disclaimer in the 
documentation and/or other materials provided with the distribution.

* Neither the name of Motorola nor the names of its contributors may be 
used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,  
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

using std::vector;

#define ALLOC_MEM 30000

mm_rtos_t alloc_mem;
float* alloc_mem_buf[ALLOC_MEM]; 

// Custom vector allocator
template <typename T>
class bufferAllocator {
  public:
    using value_type = T;
    bufferAllocator() noexcept = default;

    template <typename U>
    bufferAllocator(const bufferAllocator<U>&) noexcept {}

    T* allocate(std::size_t n)
    {
      if (auto p = static_cast<T*>(mm_rtos_malloc(&alloc_mem, n*sizeof(T), portMAX_DELAY)))
      {
        return p;
      }
    }

    void deallocate(T* p, std::size_t n) noexcept 
    {
        // TODO error checking on pointed size
        mm_rtos_free(&alloc_mem, p);
    }

    // Add required operators
    template <typename U>
    bool operator==( const bufferAllocator<U>& cmp ) noexcept 
    {
        return true;
    }

    template <typename U>
    bool operator!=( const bufferAllocator<U>& cmp ) noexcept 
    {
        return false;
    }

    static void* operator new(std::size_t size) {
        return bufferAllocator::allocate(size);
    }

    static void operator delete(void* ptr) noexcept {
        bufferAllocator::deallocate(ptr);
    }
};

/***************************************************************************/

// TODO consolidate filter classes

// Accelerometer filter 
template<typename T, int MAX_ACC_FILT_ORDER>
class accel_filter {
private:
    ButterworthHighpass<T, MAX_ACC_FILT_ORDER> accel_filt;

public:

    T filter(T accel_data) {
        accel_data = accel_filt.step(accel_data);
        return accel_data;
    }

    void designAccelFilter(int order, double cutoffFreq, double sampleFreq, bool resetCache){
        accel_filt.design(order, cutoffFreq, sampleFreq, resetCache);
    }

};

// // HRM filter 
// template<typename T, int MAX_HRM_FILT_ORDER>
// class hrm_filter {
// private:
//     ButterworthHighpass<T, MAX_ACC_FILT_ORDER> hrm_filt;

// public:

//     T filter(T hrm_data) {
//         hrm_data = hrm_filt.step(hrm_data);
//         return hrm_data;
//     }

//     void designHRMFilter(int order, double cutoffFreq, double sampleFreq, bool resetCache){
//         hrm_filt.design(order, cutoffFreq, sampleFreq, resetCache);
//     }

// };

template<typename T, int MAX_EEG_FILT_ORDER>
class eeg_filter {
private:
    ButterworthBandpass<T, MAX_EEG_FILT_ORDER> eeg_filt;

public:

    T filter(T eeg_data) {
        eeg_data = eeg_filt.step(eeg_data);
        return eeg_data;
    }

    void designEEGFilter(int order, double lowFreq, double highFreq, double sampleFreq, bool resetCache){
        eeg_filt.design(order, lowFreq, highFreq, sampleFreq, resetCache);
    }

};

/***************************************************************************/

// Resampling 
// Taken from https://github.com/terrygta/SignalResampler
template<typename T>
T sinc ( T x )
{
  if ( std::abs ( x - 0.0 ) < 0.000001 )
      return 1;
  return std::sin ( M_PI * x ) / ( M_PI * x );
}

inline int quotientCeil ( int num1, int num2 )
{
  if ( num1 % num2 != 0 )
      return num1 / num2 + 1;
  return num1 / num2;
}

template<typename T>
std::vector<T, bufferAllocator<T>> firls ( 
    int length, 
    vector<T, bufferAllocator<T>> freq, 
    const vector<T, bufferAllocator<T>>& amplitude
    )
{
    int freqSize = freq.size ();
    int weightSize = freqSize / 2;

    vector<T, bufferAllocator<T>> weight(weightSize, 1.0); 

    int filterLength = length + 1;

    for (auto &it: freq)
        it /= 2.0;

    length = ( filterLength - 1 ) / 2;
    bool Nodd = filterLength & 1;
    vector<T, bufferAllocator<T>> k( length + 1 ); 
    std::iota(k.begin(), k.end(), 0.0);
    if (!Nodd) {
        for (auto &it : k)
        it += 0.5;
    }

    T b0 = 0.0;
    if (Nodd) {
        k.erase(k.begin());
    }

    vector<T, bufferAllocator<T>> b(k.size(), 0.0); 
    for ( int i = 0; i < freqSize; i += 2 )
    {
        auto Fi = freq[i];
        auto Fip1 = freq[i+1];
        auto ampi = amplitude[i];
        auto ampip1 = amplitude[i+1];
        auto wt2 = std::pow(weight[i/2], 2);
        auto m_s = (ampip1-ampi)/(Fip1-Fi);
        auto b1 = ampi-(m_s*Fi);
        if (Nodd)
        {
            b0 += (b1*(Fip1-Fi)) + m_s/2*(std::pow(Fip1, 2)-std::pow(Fi, 2))*wt2;
        }
        std::transform(b.begin(), b.end(), k.begin(),b.begin(),
                        [m_s, Fi, Fip1, wt2](T b, T k) {
            return b + (m_s/(4*std::pow(M_PI, 2))*
            (std::cos(2*M_PI*Fip1)-std::cos(2*M_PI*Fi))/(std::pow(k, 2)))*wt2;});
        std::transform(b.begin(), b.end(), k.begin(), b.begin(),
                        [m_s, Fi, Fip1, wt2, b1](T b, T k) {
            return b + (Fip1*(m_s*Fip1+b1)*sinc<T>(2*k*Fip1) -
            Fi*(m_s*Fi+b1)*sinc<T>(2*k*Fi))*wt2;});
    }

    if (Nodd)
    {
        b.insert(b.begin(), b0); 
    }


    auto w0 = weight[0];
    vector<T, bufferAllocator<T>> a(b.size());
    std::transform(b.begin(), b.end(),
                    a.begin(),
                    [w0](T b) {return std::pow(w0, 2)*4*b;});

    vector<T, bufferAllocator<T>> result = {a.rbegin(), a.rend()};
    decltype(a.begin()) it;
    if (Nodd)
    {
        it = a.begin()+1;
    }
    else
    {
        it = a.begin();
    }
    result.insert(result.end(), it, a.end());

    for (auto &it : result) {
        it *= 0.5;
    }

    return result;
}

template<typename T>
std::vector<T, bufferAllocator<T>> kaiser ( const int order, const T bta )
{
  T Numerator, Denominator;
  Denominator = std::cyl_bessel_i(0, bta);
  auto od2 = (static_cast<T>(order)-1)/2;
  std::vector<T, bufferAllocator<T>> window;
  window.reserve(order);
  for (int n = 0; n < order; n++) {
      auto x = bta*std::sqrt(1-std::pow((n-od2)/od2, 2));
      Numerator = std::cyl_bessel_i(0, x);
      window.push_back(Numerator / Denominator);
  }
  return window;
}

template<typename T>
void resample ( int upFactor, int downFactor,
  vector<T, bufferAllocator<T>>& inputSignal, vector<T, bufferAllocator<T>>& outputSignal )
{
    const int n = 10;
    const T bta = 5.0;
    // if ( upFactor <= 0 || downFactor <= 0 )
    // throw std::runtime_error ( "factors must be positive integer" );
    int gcd_o = std::gcd ( upFactor, downFactor );
    upFactor /= gcd_o;
    downFactor /= gcd_o;

    if ( upFactor == downFactor )
    {
      outputSignal = inputSignal;
      return;
    }

    int inputSize = inputSignal.size();
    outputSignal.clear ();
    int outputSize =  quotientCeil ( inputSize * upFactor, downFactor );
    outputSignal.reserve ( outputSize );

    int maxFactor = std::max ( upFactor, downFactor );
    T firlsFreq = 1.0 / 2.0 / static_cast<T> ( maxFactor );
    int length = 2 * n * maxFactor + 1;

    vector<T, bufferAllocator<T>> firlsFreqsV = { 0.0, 2 * firlsFreq, 2 * firlsFreq, 1.0 };
    vector<T, bufferAllocator<T>> firlsAmplitudeV =  { 1.0, 1.0, 0.0, 0.0 };
    vector<T, bufferAllocator<T>> coefficients = firls<T> ( length - 1, firlsFreqsV, firlsAmplitudeV);
    vector<T, bufferAllocator<T>> window = kaiser<T> ( length, bta );

    int coefficientsSize = coefficients.size();
    for( int i = 0; i < coefficientsSize; i++ )
    {
        coefficients[i] *= upFactor * window[i];
    }
    int lengthHalf = ( length - 1 ) / 2;
    int nz = downFactor - lengthHalf % downFactor;

    vector<T, bufferAllocator<T>> h;
    h.reserve ( coefficientsSize + nz );
    for ( int i = 0; i < nz; i++ ) 
    {
        h.push_back ( 0.0 );
    }
    for ( int i = 0; i < coefficientsSize; i++ ) 
    {
        h.push_back ( coefficients[i] );
    }

    int hSize = h.size();
    lengthHalf += nz;
    int delay = lengthHalf / downFactor;
    nz = 0;

    while ( quotientCeil( ( inputSize - 1 ) * upFactor + hSize + nz, downFactor ) - delay < outputSize )
    {
        nz++;
    }

    for ( int i = 0; i < nz; i++ )
    {
        h.push_back ( 0.0 );
    }

    vector<T, bufferAllocator<T>> y;
    upfirdn ( upFactor, downFactor, inputSignal, h, y );
    
    for ( int i = delay; i < outputSize + delay; i++ )
    {
        outputSignal.push_back ( y[i] );
    }
}

template<class S1, class S2, class C>
class Resampler{
public:
    typedef    S1 inputType;
    typedef    S2 outputType;
    typedef    C coefType;

    Resampler(int upRate, int downRate, C *coefs, int coefCount);
    virtual ~Resampler();

    int        apply(S1* in, int inCount, S2* out, int outCount);
    int        neededOutCount(int inCount);
    int        coefsPerPhase() { return _coefsPerPhase; }
    
private:
    int        _upRate;
    int        _downRate;

    coefType   *_transposedCoefs;
    inputType  *_state;
    inputType  *_stateEnd;
    
    int        _paddedCoefCount;  // ceil(len(coefs)/upRate)*upRate
    int        _coefsPerPhase;    // _paddedCoefCount / upRate
    
    int        _t;                // "time" (modulo upRate)
    int        _xOffset;
    
};

using std::invalid_argument;

template<class S1, class S2, class C>
Resampler<S1, S2, C>::Resampler(int upRate, int downRate, C *coefs,
                                int coefCount):
  _upRate(upRate), _downRate(downRate), _t(0), _xOffset(0)
/*
  The coefficients are copied into local storage in a transposed, flipped
  arrangement.  For example, suppose upRate is 3, and the input number
  of coefficients coefCount = 10, represented as h[0], ..., h[9].
  Then the internal buffer will look like this:
        h[9], h[6], h[3], h[0],   // flipped phase 0 coefs
           0, h[7], h[4], h[1],   // flipped phase 1 coefs (zero-padded)
           0, h[8], h[5], h[2],   // flipped phase 2 coefs (zero-padded)
*/
{
    _paddedCoefCount = coefCount;
    while (_paddedCoefCount % _upRate) {
        _paddedCoefCount++;
    }
    _coefsPerPhase = _paddedCoefCount / _upRate;
    
    _transposedCoefs = new coefType[_paddedCoefCount];
    std::fill(_transposedCoefs, _transposedCoefs + _paddedCoefCount, 0.);

    _state = new inputType[_coefsPerPhase - 1];
    _stateEnd = _state + _coefsPerPhase - 1;
    std::fill(_state, _stateEnd, 0.);


    /* This both transposes, and "flips" each phase, while
     * copying the defined coefficients into local storage.
     * There is probably a faster way to do this
     */
    for (int i=0; i<_upRate; ++i) {
        for (int j=0; j<_coefsPerPhase; ++j) {
            if (j*_upRate + i  < coefCount)
                _transposedCoefs[(_coefsPerPhase-1-j) + i*_coefsPerPhase] =
                                                coefs[j*_upRate + i];
        }
    }
}

template<class S1, class S2, class C>
Resampler<S1, S2, C>::~Resampler() {
    delete [] _transposedCoefs;
    delete [] _state;
}

template<class S1, class S2, class C>
int Resampler<S1, S2, C>::neededOutCount(int inCount)
/* compute how many outputs will be generated for inCount inputs  */
{
    int np = inCount * _upRate;
    int need = np / _downRate;
    if ((_t + _upRate * _xOffset) < (np % _downRate))
        need++;
    return need;
}

template<class S1, class S2, class C>
int Resampler<S1, S2, C>::apply(S1* in, int inCount, 
                                S2* out, int outCount) {
    // if (outCount < neededOutCount(inCount)) 
    //     throw invalid_argument("Not enough output samples");

    // x points to the latest processed input sample
    inputType *x = in + _xOffset;
    outputType *y = out;
    inputType *end = in + inCount;
    while (x < end) {
        outputType acc = 0.;
        coefType *h = _transposedCoefs + _t*_coefsPerPhase;
        inputType *xPtr = x - _coefsPerPhase + 1;
        int offset = in - xPtr;
        if (offset > 0) {
            // need to draw from the _state buffer
            inputType *statePtr = _stateEnd - offset;
            while (statePtr < _stateEnd) {
                acc += *statePtr++ * *h++;
            }
            xPtr += offset;
        }
        while (xPtr <= x) {
            acc += *xPtr++ * *h++;
        }
        *y++ = acc;
        _t += _downRate;

        int advanceAmount = _t / _upRate;

        x += advanceAmount;
        // which phase of the filter to use
        _t %= _upRate;
    }
    _xOffset = x - end;

    // manage _state buffer
    // find number of samples retained in buffer:
    int retain = (_coefsPerPhase - 1) - inCount;
    if (retain > 0) {
        // for inCount smaller than state buffer, copy end of buffer
        // to beginning:
        std::copy(_stateEnd - retain, _stateEnd, _state);
        // Then, copy the entire (short) input to end of buffer
        std::copy(in, end, _stateEnd - inCount);
    } else {
        // just copy last input samples into state buffer
        std::copy(end - (_coefsPerPhase - 1), end, _state);
    }
    // number of samples computed
    return y - out;
}

template<class S1, class S2, class C>
void upfirdn(int upRate, int downRate, 
             float *input, int inLength, C *filter, int filterLength, 
             vector<S2, bufferAllocator<S2>> &results)
/*
This template function provides a one-shot resampling.  Extra samples
are padded to the end of the input in order to capture all of the non-zero 
output samples.
The output is in the "results" vector which is modified by the function.

Note, I considered returning a vector instead of taking one on input, but
then the C++ compiler has trouble with implicit template instantiation
(e.g. have to say upfirdn<float, float, float> every time - this
way we can let the compiler infer the template types).

Thanks to Lewis Anderson (lkanders@ucsd.edu) at UCSD for
the original version of this function.
*/
{
    // Create the Resampler
    Resampler<S1, S2, C> theResampler(upRate, downRate, filter, filterLength);

    // pad input by length of one polyphase of filter to flush all values out
    int padding = theResampler.coefsPerPhase() - 1;
    float inputPadded[inLength + padding]; // HACK
//    vector<S1, bufferAllocator<S1>> inputPadded;
//    inputPadded.reserve(inLength + padding);
    for (int i = 0; i < inLength + padding; i++) {
        if (i < inLength)
            inputPadded[i] = input[i];
        else
            inputPadded[i] = 0;
    }

    // calc size of output
    int resultsCount = theResampler.neededOutCount(inLength + padding); 

    results.resize(resultsCount);

    // run filtering
    int numSamplesComputed = theResampler.apply(inputPadded, 
            inLength + padding, &results[0], resultsCount);
    // delete[] inputPadded;
}

template<class S1, class S2, class C>
void upfirdn(int upRate, int downRate, 
             vector<S1, bufferAllocator<S1>> &input, 
             vector<C, bufferAllocator<C>> &filter, 
             vector<S2, bufferAllocator<S2>> &results)
/*
This template function provides a one-shot resampling.
The output is in the "results" vector which is modified by the function.
In this version, the input and filter are vectors as opposed to 
pointer/count pairs.
*/
{
    upfirdn<S1, S2, C>(upRate, downRate, &input[0], input.size(), &filter[0], 
                       filter.size(), results);
}

/***************************************************************************/

// Z score normalization

// Function to calculate the mean
float mean(const std::vector<float, bufferAllocator<float>>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// Function to calculate the standard deviation
float standard_deviation(const std::vector<float, bufferAllocator<float>>& v) {
    float mean_val = mean(v);
    float sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0, 
        std::plus<>(), [mean_val](float a, float b) { return (a-mean_val)*(b-mean_val); });
    return std::sqrt(sq_sum / v.size());
}

// Function to perform Z-score normalization
std::vector<float, bufferAllocator<float>> z_score_normalize(std::vector<float, bufferAllocator<float>>& v) {
    std::vector<float, bufferAllocator<float>> result;
    float mean_val = mean(v);
    float std_dev = standard_deviation(v);
    
    for(auto &value : v) {
        result.push_back((value - mean_val) / std_dev);
    }
    
    return result;
}

/***************************************************************************/

float findAbsMax(float arr[], int size) {
    int max = abs(arr[0]);  // Assuming the first element is the maximum

    for (int i = 1; i < size; i++) {
        if (abs(arr[i]) > max) {
            max = abs(arr[i]);  // Update max if a larger element is found
        }
    }

    return max;
}

#endif // ML_UTILS_H
