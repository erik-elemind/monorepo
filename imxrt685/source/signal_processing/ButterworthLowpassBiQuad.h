
#ifndef _BUTTERWORTH_LOWPASS_BIQUAD_
#define _BUTTERWORTH_LOWPASS_BIQUAD_

// Copied from: https://www.dsprelated.com/showarticle/1137.php
// Copied on: October 10th, 2020
// Ported to C++ by David Wang

// % biquad_synth.m    2/10/18 Neil Robertson
// % Synthesize even-order IIR Butterworth lowpass filter as cascaded biquads.
// % This function computes the denominator coefficients a of the biquads.
// % N= filter order (must be even)
// % fc= -3 dB frequency in Hz
// % fs= sample frequency in Hz
// % a = matrix of denominator coefficients of biquads.  Size = (N/2,3)
// %     each row of a contains the denominator coeffs of a biquad.
// %     There are N/2 rows.
// % Note numerator coeffs of each biquad= K*[1 2 1], where K = (1 + a1 + a2)/4.
// %
// function a = biquad_synth(N,fc,fs);
// if fc>=fs/2;
//   error('fc must be less than fs/2')
// end
// if mod(N,2)~=0
//     error('N must be even')
// end
// %I.  Find analog filter poles above the real axis (half of total poles)
// k= 1:N/2;
// theta= (2*k -1)*pi/(2*N);
// pa= -sin(theta) + j*cos(theta);     % poles of filter with cutoff = 1 rad/s
// pa= fliplr(pa);                  %reverse sequence of poles – put high Q last
// % II.  scale poles in frequency
// Fc= fs/pi * tan(pi*fc/fs);          % continuous pre-warped frequency
// pa= pa*2*pi*Fc;                     % scale poles by 2*pi*Fc
// % III.  Find coeffs of biquads
// % poles in the z plane
// p= (1 + pa/(2*fs))./(1 - pa/(2*fs));      % poles by bilinear transform
// % denominator coeffs 
// for k= 1:N/2;
//     a1= -2*real(p(k));
//     a2= abs(p(k))^2;
//     a(k,:)= [1 a1 a2];             %coeffs of biquad k
// end

#include <cmath>
#include "math_util.h"
// #include <complex>

template<typename T>
class Complex {
public:
	T re;
	T im;
    Complex(T real, T imag): re(real), im(imag){
    }
    static Complex<T> mult(Complex<T> c, T s);
    // static Complex<T> mult(T s, Complex<T> c);
    // static Complex<T> mult(Complex<T> c1, Complex<T> c2);
    static Complex<T> div(Complex<T> c, T s);
    static Complex<T> div(Complex<T> c1, Complex<T> c2);
    // static Complex<T> sub(Complex<T> c, T s);
    static Complex<T> sub(T s, Complex<T> c);
    // static Complex<T> sub(Complex<T> c1, Complex<T> c2);
    // static Complex<T> add(Complex<T> c, T s);
    static Complex<T> add(T s, Complex<T> c);
    // static Complex<T> add(Complex<T> c1, Complex<T> c2);
    // static T abs(Complex<T> c);
};

template<typename T>
Complex<T> Complex<T>::mult(Complex<T> c, T s){
    return Complex(s*c.re, s*c.im);
}
// template<typename T>
// Complex<T> Complex<T>::mult(T s, Complex<T> c){
//     return Complex(s*c.re, s*c.im);
// }
// template<typename T>
// Complex<T> Complex<T>::mult(Complex<T> c1, Complex<T> c2){
//     return Complex(c1.re*c2.re - c1.im*c2.im, 
//                    c1.re*c2.im + c1.im*c2.re);
// }
template<typename T>
Complex<T> Complex<T>::div(Complex<T> c, T s){
    return Complex(c.re/s, c.im/s);
}
template<typename T>
Complex<T> Complex<T>::div(Complex<T> c1, Complex<T> c2){
    T den = c2.re*c2.re + c2.im*c2.im;
    return Complex((c1.re*c2.re+c1.im*c2.im)/den, (c1.im*c2.re-c1.re*c2.im)/den);
}
// template<typename T>
// Complex<T> Complex<T>::sub(Complex<T> c, T s){
//     return Complex(c.re-s, c.im);
// }
template<typename T>
Complex<T> Complex<T>::sub(T s, Complex<T> c){
    return Complex(s-c.re,-c.im);
}
// template<typename T>
// Complex<T> Complex<T>::sub(Complex<T> c1, Complex<T> c2){
//     return Complex(c1.re-c2.re, c1.im-c2.im);
// }
// template<typename T>
// Complex<T> Complex<T>::add(Complex<T> c, T s){
//     return Complex(c.re+s, c.im);
// }
template<typename T>
Complex<T> Complex<T>::add(T s, Complex<T> c){
    return Complex(s+c.re, c.im);
}
// template<typename T>
// Complex<T> Complex<T>::add(Complex<T> c1, Complex<T> c2){
//     return Complex(c1.re+c2.re, c1.im+c2.im);
// }
// template<typename T>
// T Complex<T>::abs(Complex<T> c){
//     return sqrt(c.re*c.re + c.im*c.im);
// }

enum ButterworthLowpassStatus {STATUS_BL_OK=0, STATUS_BL_FC2BIG, STATUS_BL_NEVEN};

template<typename T, int MAX_BB_ORDER_T>
class ButterworthLowpassBiQuad{
private:
    size_t N;

    T k[MAX_BB_ORDER_T/2+1];
    T a[MAX_BB_ORDER_T/2+1][3];
    T b[MAX_BB_ORDER_T/2+1][3];
    T v[MAX_BB_ORDER_T/2+1][3];
    size_t v_i;

    T step(T val, T k, T* a, T* b, T *v, size_t v_i){
        // compute the previous indices
        size_t v_0 = v_i;
        size_t v_1 = (v_i+2)%3;
        size_t v_2 = (v_i+1)%3;
        v[v_0] = k*val - a[1]*v[v_1] - a[2]*v[v_2];
        return b[0]*v[v_0] + b[1]*v[v_1] + b[2]*v[v_1];
    }

public:
    ButterworthLowpassBiQuad(){
    }

    int design(size_t N, double fc, double fs, bool resetCache) {
        if (fc >= fs/2) {
            // error("fc must be less than fs/2");
            return STATUS_BL_FC2BIG;
        }
        if (N%2 != 0) {
            // error("N must be even");
            return STATUS_BL_NEVEN;
        }
        // save off the order
        this->N = N;
        for (size_t i=0; i<N/2; i++) {
            //I.  Find analog filter poles above the real axis (half of total poles)
            size_t i_rev = N/2-i;              // reverse sequence of poles – put high Q last
            T theta = (2*i_rev -1)*M_PI/(2*N);
            Complex<T> pa( -sin(theta) , cos(theta) );     // poles of filter with cutoff = 1 rad/s       
            // II.  scale poles in frequency
            T Fc = fs/M_PI * tan(M_PI*fc/fs);          // continuous pre-warped frequency
            pa = Complex<T>::mult( pa , 2*M_PI*Fc );                     // scale poles by 2*pi*Fc
            // III.  Find coeffs of biquads
            // poles in the z plane
            Complex<T> p_num = Complex<T>::add(1 , Complex<T>::div(pa,(2*fs)));
            Complex<T> p_den = Complex<T>::sub(1 , Complex<T>::div(pa,(2*fs)));
            Complex<T> p = Complex<T>::div(p_num,p_den);      // poles by bilinear transform
            // denominator coeffs 
            double a1 = -2 * p.re;
            double a2 = p.re*p.re + p.im*p.im;
            a[i][0] = 1;             // coeffs of biquad k
            a[i][1] = a1;
            a[i][2] = a2;
            // numerator coeffs 
            b[i][0] = 1;
            b[i][1] = 2;
            b[i][2] = 1;
            // biquad gain
            k[i] = (1+a1+a2)/4.0;
        }
        if(resetCache){
            reset();
        }
        return STATUS_BL_OK;
    }

    T step(T val){
        for(size_t i=0; i<N/2; i++){
            val = step(val, k[i], a[i], b[i], v[i], v_i);
        }
        // move to the next entry in v
        v_i = (v_i+1) % 3;
        return val;
    }

    void reset(){
        memset(v,0,sizeof(v));
    }

//    void printCoeffs(Stream &s)
//    {
//        s.println("BiQuad Coeffs: ");
//        for (size_t i = 0; i < N / 2; i++)
//        {
//            // print gain
//            s.print("k = ");
//            s.println(k[i],10);
//            // print a
//            s.print("a = ");
//            for (int ai = 0; ai < 3; ai++)
//            {
//                s.print(a[i][ai], 10);
//                s.print(", ");
//            }
//            s.println();
//            // print b
//            s.print("b = ");
//            for (int bi = 0; bi < 3; bi++)
//            {
//                s.print(b[i][bi], 10);
//                s.print(", ");
//            }
//            s.println();
//        }
//    }


};


#endif //_BUTTERWORTH_LOWPASS_BIQUAD_
