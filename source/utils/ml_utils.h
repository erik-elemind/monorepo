#ifndef ML_UTILS_H
#define ML_UTILS_H

// filters
// resample
// z score normalization

//TODO fix vector init to static at init

#include "ml.h"
#include "ButterworthHighpass.h"
// #include "ButterworthBandpass.h"

#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include "upfirdn.h"
#include "memman_rtos.h"

using std::vector;

// more globals yay
static float* alloc_mem_buf[10000];
static mm_rtos_t alloc_mem; 

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
      if (auto p = static_cast<T*>(mm_rtos_malloc(&alloc_mem, n, portMAX_DELAY)))
      {
        return p;
      }
    }

    void deallocate(T* p, std::size_t n) noexcept {
        // TODO error checking on pointed size
        mm_rtos_free(&alloc_mem, p);
    }

};


// Accelerometer filter 
template<typename T, int MAX_ACC_FILT_ORDER>
class accel_filter {
private:
    ButterworthHighpass<T,MAX_ACC_FILT_ORDER> accel_filt;

public:

    T filter(T accel_data) {
        accel_data = accel_filt.step(accel_data);
        return accel_data;
    }

    void designAccelFilter(int order, double cutoffFreq, double sampleFreq, bool resetCache){
        accel_filt.design(order, cutoffFreq, sampleFreq, resetCache);
    }

};

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
std::vector<T> firls ( int length, vector<T> freq, const vector<T>& amplitude)
{
  int freqSize = freq.size ();
  int weightSize = freqSize / 2;

  vector<T> weight(weightSize, 1.0);

  int filterLength = length + 1;

  for (auto &it: freq)
      it /= 2.0;

  length = ( filterLength - 1 ) / 2;
  bool Nodd = filterLength & 1;
  vector<T> k( length + 1 );
  std::iota(k.begin(), k.end(), 0.0);
  if (!Nodd) {
    for (auto &it : k)
      it += 0.5;
  }

  T b0 = 0.0;
  if (Nodd) {
      k.erase(k.begin());
  }

  vector<T> b(k.size(), 0.0);
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
  vector<T> a(b.size());
  std::transform(b.begin(), b.end(),
                 a.begin(),
                 [w0](T b) {return std::pow(w0, 2)*4*b;});

  vector<T> result = {a.rbegin(), a.rend()};
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
std::vector<T> kaiser ( const int order, const T bta )
{
  T Numerator, Denominator;
  Denominator = std::cyl_bessel_i(0, bta);
  auto od2 = (static_cast<T>(order)-1)/2;
  std::vector<T> window;
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
  vector<T>& inputSignal, vector<T>& outputSignal )
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

    vector<T> firlsFreqsV = { 0.0, 2 * firlsFreq, 2 * firlsFreq, 1.0 };
    vector<T> firlsAmplitudeV =  { 1.0, 1.0, 0.0, 0.0 };
    vector<T> coefficients = firls<T> ( length - 1, firlsFreqsV, firlsAmplitudeV);
    vector<T> window = kaiser<T> ( length, bta );

    int coefficientsSize = coefficients.size();
    for( int i = 0; i < coefficientsSize; i++ )
    {
        coefficients[i] *= upFactor * window[i];
    }
    int lengthHalf = ( length - 1 ) / 2;
    int nz = downFactor - lengthHalf % downFactor;

    vector<T> h;
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

    vector<T> y;
    upfirdn ( upFactor, downFactor, inputSignal, h, y );
    
    for ( int i = delay; i < outputSize + delay; i++ )
    {
        outputSignal.push_back ( y[i] );
    }
}

// Z score normalization

// Function to calculate the mean
float mean(const std::vector<float>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// Function to calculate the standard deviation
float standard_deviation(const std::vector<float>& v) {
    float mean_val = mean(v);
    float sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0, 
        std::plus<>(), [mean_val](float a, float b) { return (a-mean_val)*(b-mean_val); });
    return std::sqrt(sq_sum / v.size());
}

// Function to perform Z-score normalization
std::vector<float> z_score_normalize(std::vector<float>& v) {
    std::vector<float> result;
    float mean_val = mean(v);
    float std_dev = standard_deviation(v);
    
    for(auto &value : v) {
        result.push_back((value - mean_val) / std_dev);
    }
    
    return result;
}



#endif // ML_UTILS_H